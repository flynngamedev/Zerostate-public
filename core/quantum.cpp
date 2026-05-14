#include "quantum.h"
#include "entropy.h"
#include <cstring>

namespace quantum {

void pool_init(Pool& p) {
    for (int i = 0; i < POOL_HALF; i++)  p.bits[i] = 0;
    for (int i = POOL_HALF; i < POOL_SIZE; i++) p.bits[i] = 1;

    for (int i = 0; i < POOL_SIZE; i++) p.indices[i] = i;

    for (int i = POOL_SIZE - 1; i > 0; i--) {
        uint64_t r = 0;
        entropy::rdseed64(r);
        int j = (int)(r % (uint64_t)(i + 1));
        int tmp      = p.indices[i];
        p.indices[i] = p.indices[j];
        p.indices[j] = tmp;
    }

    p.head = 0;
}

void pool_take(Pool& p, uint8_t* out, int count) {
    while (p.lock.test_and_set(std::memory_order_acquire));
    for (int i = 0; i < count; i++) {
        int idx = p.indices[p.head];
        out[i]  = p.bits[idx];
        p.head  = (p.head + 1) % POOL_SIZE;
    }
    p.lock.clear(std::memory_order_release);
}

void pool_return(Pool& p, const uint8_t* bits, int count) {
    while (p.lock.test_and_set(std::memory_order_acquire));
    for (int i = 0; i < count; i++) {
        int tail    = (p.head + i) % POOL_SIZE;
        int idx     = p.indices[tail];
        p.bits[idx] = bits[i];
    }
    p.lock.clear(std::memory_order_release);
}

} // namespace quantum
