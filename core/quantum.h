#pragma once
#include <cstdint>
#include <cstddef>
#include <atomic>

// ─── ZeroState Quantum Pool ────────────────────────────────────────────────
// A circular bucket of 5000 zeros and 5000 ones.
// Used as a fallback when RDSEED is unavailable (CPU too busy).
//
// Rules:
//   - Pool is ALWAYS exactly 10000 bits — never grows, never shrinks.
//   - 50/50 balance is PERMANENTLY preserved — you return what you take.
//   - RDSEED is only used at init time to shuffle the pool.

#define POOL_SIZE  10000
#define POOL_HALF  5000

namespace quantum {

    struct Pool {
        uint8_t  bits[POOL_SIZE];
        int      indices[POOL_SIZE];
        int      head;
        std::atomic_flag lock = ATOMIC_FLAG_INIT;
    };

    // Initialise the pool. Seeds shuffle via RDSEED.
    void pool_init(Pool& p);

    // Take `count` bits from the pool and copy to `out`.
    // Immediately returns them — caller must call pool_return after use.
    void pool_take(Pool& p, uint8_t* out, int count);

    // Return `count` bits back into the pool.
    // Pool size never changes — 50/50 balance is preserved.
    void pool_return(Pool& p, const uint8_t* bits, int count);

} // namespace quantum
