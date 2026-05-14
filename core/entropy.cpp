#include "entropy.h"
#include <cstdint>
#include <intrin.h>      // MSVC intrinsics -- _rdseed64_step, __rdtsc
#include <windows.h>     // QueryPerformanceCounter

namespace entropy {

// ── Raw RDSEED via MSVC intrinsic ─────────────────────────────────────────
// MSVC does not support GNU __asm__ volatile syntax.
// _rdseed64_step is the MSVC/ICC intrinsic equivalent.

void rdseed64_asm(uint64_t& out) {
    unsigned long long val = 0;
    while (!_rdseed64_step(&val));
    out = (uint64_t)val;
}

bool rdseed64(uint64_t& out, int retries) {
    unsigned long long val = 0;
    for (int i = 0; i < retries; i++) {
        if (_rdseed64_step(&val)) {
            out = (uint64_t)val;
            return true;
        }
    }
    return false;
}

// ── 86-bit entropy ────────────────────────────────────────────────────────

u86 rdseed86() {
    u86 val;
    uint64_t raw = 0;
    rdseed64_asm(val.lo);
    rdseed64_asm(raw);
    val.hi = raw >> 42;
    return val;
}

// ── 86-bit to double conversion ───────────────────────────────────────────

static const double POW2_22 = (double)(1ULL << 22);
static const double POW2_64 = 18446744073709551616.0;
static const double POW2_86 = POW2_22 * POW2_64;

double u86_to_velocity(u86 val) {
    double pct = (double)val.hi / POW2_22
               + (double)val.lo / POW2_86;
    return pct * 100.0;
}

double u86_to_range(u86 val, double range) {
    double pct = (double)val.hi / POW2_22
               + (double)val.lo / POW2_86;
    return pct * range;
}

// ── Timing jitter via QueryPerformanceCounter ─────────────────────────────
// CLOCK_MONOTONIC does not exist on MSVC Windows.
// QueryPerformanceCounter is the Windows equivalent.

uint64_t timing_jitter() {
    LARGE_INTEGER t1, t2;
    uint64_t jitter = 0;

    for (int i = 0; i < 8; i++) {
        QueryPerformanceCounter(&t1);

        volatile uint64_t sink = 0;
        for (int j = 0; j < 1000; j++) sink ^= (uint64_t)j * 6364136223846793005ULL;
        (void)sink;

        QueryPerformanceCounter(&t2);

        uint64_t delta = (uint64_t)(t2.QuadPart - t1.QuadPart);
        jitter = (jitter << 8) ^ delta;
    }

    return jitter;
}

uint64_t rdseed_xor_jitter() {
    uint64_t seed = 0;
    rdseed64(seed);
    return seed ^ timing_jitter();
}

} // namespace entropy
