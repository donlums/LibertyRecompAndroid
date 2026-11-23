// GTA IV Android: bounded producer-side frame cadence profiler.
// Title-to-renderer submission cadence is NOT a Vulkan GPU or display-present measurement.
#pragma once

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <optional>

namespace gta4::diagnostics {

struct FrameHandoffWindow {
  uint32_t count = 0;
  uint32_t gaps = 0;
  uint32_t gaps_over_50ms = 0;
  uint32_t gaps_over_66ms = 0;
  uint64_t avg_gap_us = 0;
  uint64_t max_gap_us = 0;
  uint64_t avg_submit_us = 0;
  uint64_t max_submit_us = 0;
};

// A fixed 120-submission window with no allocation or logging on the hot path.
// A single mutex allows main and worker present producers to share the summary.
class FrameHandoffProfiler final {
 public:
  static constexpr uint32_t kWindowSize = 120;

  std::optional<FrameHandoffWindow> Observe(uint64_t submit_start_us,
                                            uint64_t submit_duration_us) {
    std::lock_guard lock(mutex_);
    if (last_start_us_ && submit_start_us >= last_start_us_) {
      const uint64_t gap_us = submit_start_us - last_start_us_;
      // Exclude a pause/background-to-foreground transition.
      if (gap_us <= 2'000'000) {
        ++gaps_;
        gap_total_us_ += gap_us;
        max_gap_us_ = std::max(max_gap_us_, gap_us);
        gaps_over_50ms_ += (gap_us > 50'000);
        gaps_over_66ms_ += (gap_us > 66'666);
      }
    }
    last_start_us_ = submit_start_us;
    ++count_;
    submit_total_us_ += submit_duration_us;
    max_submit_us_ = std::max(max_submit_us_, submit_duration_us);
    if (count_ != kWindowSize) return std::nullopt;
    FrameHandoffWindow result{
        .count = count_,
        .gaps = gaps_,
        .gaps_over_50ms = gaps_over_50ms_,
        .gaps_over_66ms = gaps_over_66ms_,
        .avg_gap_us = gaps_ ? gap_total_us_ / gaps_ : 0,
        .max_gap_us = max_gap_us_,
        .avg_submit_us = submit_total_us_ / count_,
        .max_submit_us = max_submit_us_,
    };
    count_ = gaps_ = gaps_over_50ms_ = gaps_over_66ms_ = 0;
    gap_total_us_ = max_gap_us_ = submit_total_us_ = max_submit_us_ = 0;
    return result;
  }

 private:
  std::mutex mutex_;
  uint64_t last_start_us_ = 0;
  uint32_t count_ = 0, gaps_ = 0;
  uint32_t gaps_over_50ms_ = 0, gaps_over_66ms_ = 0;
  uint64_t gap_total_us_ = 0, max_gap_us_ = 0;
  uint64_t submit_total_us_ = 0, max_submit_us_ = 0;
};

}  // namespace gta4::diagnostics
