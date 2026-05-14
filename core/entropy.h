#pragma once
#include <cstdint>
#include <cstddef>

// ─── ZeroState Entropy Layer ───────────────────────────────────────────────
// MSVC version -- uses _rdseed64_step intrinsic and QueryPerformanceCounter.

namespace entropy {

    struct u86 {
        uint64_t lo;
        uint64_t hi;  // top 22 bits only
    };

    void     rdseed64_asm(uint64_t& out);
    bool     rdseed64(uint64_t& out, int retries = 10);
    u86      rdseed86();
    double   u86_to_velocity(u86 val);
    double   u86_to_range(u86 val, double range);
    uint64_t timing_jitter();
    uint64_t rdseed_xor_jitter();

} // namespace entropy
