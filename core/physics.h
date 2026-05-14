#pragma once
#include <cstdint>

// ─── ZeroState Physics Engine ──────────────────────────────────────────────
// Simulates a ball bouncing in a 3D box with 20 spherical spike obstacles.
//
// Entropy sources:
//   V          — velocity magnitude from RDSEED (never truly 0 or 100 mph)
//   StartPos   — X,Y,Z from 3x RDSEED
//   Spikes     — 20x (RDSEED XOR jitter) for position + radius
//
// The simulation is deterministic given its inputs — it adds no new entropy.
// Its role is MIXING: spreading input entropy across 240 bounce timestamps.
//
// Output: 240 nanosecond timestamps packed into 2400 bytes.
// Distilled: XOR first half with second half → 1200 bytes → SHA-3 → uint64_t

#define NUM_SPIKES   20
#define NUM_BOUNCES  240
#define BOX_SIZE     100.0

namespace physics {

    struct Vec3 {
        double x, y, z;
    };

    struct Spike {
        Vec3   pos;
        double radius;
    };

    struct SimResult {
        uint64_t timestamps[NUM_BOUNCES]; // nanosecond bounce timestamps
    };

    // Explicit seed for deterministic replay (used by --trace)
    struct SimSeed {
        double velocity;   // mph (0-100)
        Vec3   start;      // X, Y, Z start position
    };

    // Full simulation state -- used by the visualiser
    struct SimState {
        Vec3   pos;                  // current ball position
        Vec3   vel;                  // current ball velocity
        Spike  spikes[NUM_SPIKES];   // spike positions and radii
        int    bounce;               // current bounce count
        double speed;                // initial speed (mph)
    };

    // Seed and run the full simulation.
    // Returns 240 bounce timestamps.
    SimResult run_simulation();

    // Run simulation with explicit seed -- deterministic replay.
    SimResult run_simulation_seeded(const SimSeed& seed);

    // Initialise a SimState from RDSEED entropy -- for visualiser.
    SimState  init_state();

    // Advance simulation by one tick -- for visualiser.
    // Returns true if a bounce occurred this tick.
    bool      tick(SimState& state);

    // Distil 240 timestamps into a single uint64_t via XOR folding + SHA-3.
    uint64_t distil(const SimResult& result);

    // Convert a double velocity back to its 86-bit binary string representation.
    void velocity_to_bits(double vel, char out[87]); // 86 chars + null

} // namespace physics
