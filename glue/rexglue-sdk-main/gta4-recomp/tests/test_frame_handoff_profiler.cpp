// Host-only smoke test: clang++ -std=c++23 -O2 -Wall -Wextra -Werror -I../src test_frame_handoff_profiler.cpp -o test && ./test
#include "../src/gta4_frame_handoff_profiler.h"
#include <cassert>
#include <cstdint>
#include <iostream>

using gta4::diagnostics::FrameHandoffProfiler;

int main() {
  FrameHandoffProfiler probe;
  uint64_t now = 1'000'000;
  for (int i = 0; i < 119; ++i) {
    assert(!probe.Observe(now, 1'500));
    now += 33'333;
  }
  auto normal = probe.Observe(now, 1'500);
  assert(normal && normal->count == 120 && normal->gaps == 119);
  assert(normal->gaps_over_50ms == 0 && normal->avg_submit_us == 1'500);

  for (int i = 0; i < 119; ++i) {
    now += 66'667;
    assert(!probe.Observe(now, 9'000));
  }
  now += 66'667;
  auto slow = probe.Observe(now, 9'000);
  assert(slow && slow->gaps_over_50ms == 120 && slow->gaps_over_66ms == 120);
  assert(slow->avg_gap_us == 66'667 && slow->avg_submit_us == 9'000);
  std::cout << "PASS: 30/15 FPS cadence and submit timing" << std::endl;
}
