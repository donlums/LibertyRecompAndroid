// Build: c++ -std=c++23 -O2 -Wall -Wextra -Werror -Iglue/rexglue-sdk-main/src/graphics/gta4_native
//        tools/tests/native_index_binding_cache_test.cpp -o /tmp/native-index-test && /tmp/native-index-test
#include "native_draw_state_cache.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>

using rex::graphics::gta4_native::NativeDrawStateCache;

int main() {
  NativeDrawStateCache<2> cache;
  // New binding -> emit; exact repetition -> suppress.
  assert(cache.UpdateIndexBuffer(101, 200, 0, 1));
  assert(!cache.UpdateIndexBuffer(101, 200, 0, 1));
  // Offset, buffer, type and command-buffer identity all require a new command.
  assert(cache.UpdateIndexBuffer(101, 200, 8, 1));
  assert(cache.UpdateIndexBuffer(101, 201, 8, 1));
  assert(cache.UpdateIndexBuffer(101, 201, 8, 0));
  assert(cache.UpdateIndexBuffer(102, 201, 8, 0));
  assert(!cache.UpdateIndexBuffer(102, 201, 8, 0));
  // Pipeline switches must NOT cause redundant index binding. Vulkan keeps it.
  assert(cache.UpdatePipeline(350));
  assert(!cache.UpdateIndexBuffer(102, 201, 8, 0));
  assert(cache.UpdatePipeline(351));
  assert(!cache.UpdateIndexBuffer(102, 201, 8, 0));
  // Reset must invalidate all state when the recording or external path changes.
  cache.Reset();
  assert(cache.UpdateIndexBuffer(102, 201, 8, 0));
  assert(!cache.UpdateIndexBuffer(102, 201, 8, 0));

  // Structural randomized oracle: cache emissions always match a correct
  // Vulkan command-buffer binding state, including interspersed resets.
  std::mt19937_64 rng(0x4A17BEEFULL);
  std::array<uint64_t, 4> known{};
  bool valid = false;
  for (int i = 0; i < 100000; ++i) {
    if (rng() % 61 == 0) {
      cache.Reset();
      valid = false;
      continue;
    }
    if (rng() % 13 == 0) {
      cache.UpdatePipeline(rng() % 23);
    }
    // Model one logical command stream, with a reset whenever a new buffer
    // recording starts. Unchanged identity across calls can be omitted.
    const std::array<uint64_t, 4> state{
        100 + rng() % 2, 200 + rng() % 7, (rng() % 4) * 16, rng() % 3
    };
    const bool should_emit = !valid || known != state;
    const bool emit = cache.UpdateIndexBuffer(state[0], state[1], state[2],
                                               static_cast<uint32_t>(state[3]));
    assert(emit == should_emit);
    known = state;
    valid = true;
  }
  std::cout << "PASS native Vulkan index binding dedup: resets, identity, index type, pipeline, stress\n";
}
