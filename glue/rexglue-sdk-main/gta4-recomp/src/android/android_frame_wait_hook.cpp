// sub_82A46D70(device r3, out r4) writes {0, submitted frame, submitted -
// completed} for the device. Its only caller, sub_828BF420, polls it in a
// loop without sleeping while two frames are pending, i.e. while the title
// waits for the renderer to submit a frame: a busy-wait that held a big core
// at ~7% of the main game thread's samples. Sleep briefly when the title is
// about to spin; the game's own loop still decides when to stop waiting.
//
// The same two-frame limit caps throughput: one frame passes the command
// worker, the recorder and the GPU in sequence (~75 ms at the bridge), so two
// in flight give ~38 ms per frame while every stage has slack. Reporting
// fewer pending frames lets the title run that many frames further ahead; the
// renderer's own queue still bounds the total.
#include <time.h>
#include <atomic>
#include <chrono>
#include <android/log.h>
#include "android_frame_metrics.h"

#include <rex/cvar.h>

#include "gta4_init.h"

// No frame rate change, but the spin held the main game thread at ~97% of a
// big core while it waited (17% with the sleep): the whole process used 2.16
// cores at the save point with the spin and 1.37 without (Retroid Pocket 5).
// It was off for a while because the game-code crash in sub_8296E480 (stale
// object pointer) first appeared in builds that had it; that crash has its
// own guard now.
REXCVAR_DEFINE_BOOL(gta4_frame_wait_sleep, true, "GTA IV/Performance",
                    "Sleep instead of spinning while the title waits for a frame slot");
REXCVAR_DEFINE_UINT32(gta4_extra_frames_in_flight, 0, "GTA IV/Performance",
                      "Frames the title may run ahead beyond its own limit of two pending "
                      "frames (more throughput, one frame of input latency each)")
    .range(0, 2);

REXCVAR_DEFINE_BOOL(gta4_android_frame_metrics, false, "GTA IV/Diagnostics",
                    "Show guest completed-frame rate and log two-second cadence samples");
namespace gta4::android_metrics {
std::atomic<unsigned> completed_fps_tenths{0};
}
extern "C" void sub_82A46D70(PPCContext& ctx, uint8_t* base) {
  const uint32_t out = ctx.r4.u32;
  __imp__sub_82A46D70(ctx, base);
  uint32_t pending = REX_LOAD_U32(out + 8);
  if (pending >= 0x80000000u) return;
  if (REXCVAR_GET(gta4_android_frame_metrics)) {
    using Clock = std::chrono::steady_clock;
    static thread_local auto start = Clock::now();
    static thread_local uint32_t previous = REX_LOAD_U32(out + 4) - pending;
    static thread_local uint32_t peak_pending = 0;
    static thread_local uint64_t polls = 0;
    const uint32_t completed = REX_LOAD_U32(out + 4) - pending;
    if (pending > peak_pending) peak_pending = pending;
    ++polls;
    const auto now = Clock::now();
    const double elapsed = std::chrono::duration<double>(now - start).count();
    if (elapsed >= 2.0) {
      const uint32_t delta = completed - previous;
      // A device reset or counter wrap must not produce a spurious FPS reading.
      if (delta < 1000 && elapsed < 15.0) {
        const double fps = delta / elapsed;
        gta4::android_metrics::completed_fps_tenths.store(static_cast<unsigned>(fps * 10.0 + 0.5),
            std::memory_order_relaxed);
        __android_log_print(ANDROID_LOG_WARN, "LibertyRecomp",
            "frame-cadence: guest-completed=%.1f fps frames=%u elapsed=%.2fs pending-peak=%u polls=%llu",
            fps, delta, elapsed, peak_pending, static_cast<unsigned long long>(polls));
      } else {
        gta4::android_metrics::completed_fps_tenths.store(0, std::memory_order_relaxed);
      }
      start = now; previous = completed; peak_pending = 0; polls = 0;
    }
  }
  const uint32_t extra = REXCVAR_GET(gta4_extra_frames_in_flight);
  if (extra) {
    pending = pending > extra ? pending - extra : 0;
    REX_STORE_U32(out + 8, pending);
  }
  if (pending >= 2 && REXCVAR_GET(gta4_frame_wait_sleep)) {
    const timespec delay{0, 250 * 1000};
    nanosleep(&delay, nullptr);
  }
}
