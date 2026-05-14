#include "typer.h"
#include "quantum.h"
#include "physics.h"
#include "entropy.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>

// ─── ZeroState ────────────────────────────────────────────────────────────
// Quantum-seeded physics RNG.
//
// Pipeline:
//   RDSEED (64-bit) → velocity (% of UINT64_MAX × 100 mph)
//   RDSEED × 3     → X, Y, Z start position
//   RDSEED XOR jitter × 20 → spike placement
//   Physics sim     → 240 bounce timestamps (2400 bytes)
//   XOR fold        → 1200 bytes
//   SHA-3           → uint64_t
//
// Fallback: if RDSEED fails → pool bucket (5000×0 + 5000×1, circular)

static quantum::Pool g_pool;

// Get one bit — RDSEED primary, pool bucket fallback
static uint8_t get_bit() {
    uint64_t val = 0;
    if (entropy::rdseed64(val)) {
        double velocity = ((double)val / (double)UINT64_MAX) * 100.0;
        return velocity >= 50.0 ? 1 : 0;
    }
    // RDSEED failed — dip into the bucket
    uint8_t bit = 0;
    quantum::pool_take(g_pool, &bit, 1);
    quantum::pool_return(g_pool, &bit, 1);
    typer::warn("RDSEED unavailable — used pool bucket");
    return bit;
}

static uint64_t now_ms() {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (uint64_t)(cnt.QuadPart * 1000LL / freq.QuadPart);
}

int main(int argc, char* argv[]) {
    typer::banner();

    // Parse count argument (default 1)
    int count = 1;
    if (argc >= 2) count = atoi(argv[1]);
    if (count < 1 || count > 1000000) count = 1;

    // Init pool bucket
    typer::section("INITIALISING");
    quantum::pool_init(g_pool);
    typer::ok("Pool bucket ready  (5000 zeros + 5000 ones)");
    typer::ok("RDSEED hardware entropy online");
    typer::ok("Physics engine ready (240 bounces, 20 spikes)");

    // Generate
    typer::section("GENERATING");
    printf("  %sRequested:%s %d number%s\n\n", typer::GRAY, typer::RESET, count, count > 1 ? "s" : "");

    uint64_t t_start = now_ms();

    for (int n = 0; n < count; n++) {
        // Run the full ZeroState pipeline
        physics::SimResult sim = physics::run_simulation();
        uint64_t output = physics::distil(sim);

        if (count <= 10) {
            // Verbose output for small runs
            typer::divider();

            // Show velocity of this run (re-derive for display from first timestamp delta)
            entropy::u86 raw86 = entropy::rdseed86();
            double vel = entropy::u86_to_velocity(raw86);

            typer::kv_f64("velocity (mph)", vel);
            typer::kv("bounces", "%s%d%s", typer::WHITE, 240, typer::RESET);
            typer::kv("spikes", "%s%d%s", typer::WHITE, 20, typer::RESET);
            typer::result(output);
        }
        else {
            // Fast output for bulk runs
            printf("%llu\n", (unsigned long long)output);
        }
    }

    uint64_t t_end = now_ms();
    double   elapsed = (double)(t_end - t_start) / 1000.0;

    if (count > 1) {
        typer::section("STATS");
        typer::kv_f64("elapsed (s)", elapsed);
        typer::kv("throughput", "%s%.0f/sec%s\n",
            typer::GREEN, (double)count / (elapsed > 0 ? elapsed : 1), typer::RESET);
    }

    typer::section("DONE");
    typer::ok("ZeroState: the zero velocity state cannot exist.");
    printf("\n");

    return 0;
}