#include "physics.h"
#include "entropy.h"
#include <cmath>
#include <cstring>
#include <windows.h>     // QueryPerformanceCounter

namespace physics {

// ── Helpers ──────────────────────────────────────────────────────────────

static double rdseed_to_range(double lo, double hi) {
    entropy::u86 val = entropy::rdseed86();
    return lo + entropy::u86_to_range(val, hi - lo);
}

static uint64_t now_ns() {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (uint64_t)(cnt.QuadPart * 1000000000LL / freq.QuadPart);
}

static double dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

static Vec3 sub(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }

static double mag(Vec3 v) { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }

static Vec3 norm(Vec3 v) {
    double m = mag(v);
    if (m < 1e-12) return {1,0,0};
    return {v.x/m, v.y/m, v.z/m};
}

// ── Simulation ───────────────────────────────────────────────────────────

SimResult run_simulation() {
    SimResult result{};

    double speed = rdseed_to_range(0.0, 100.0);
    Vec3 dir = {
        rdseed_to_range(-1.0, 1.0),
        rdseed_to_range(-1.0, 1.0),
        rdseed_to_range(-1.0, 1.0)
    };
    dir = norm(dir);
    Vec3 vel = { dir.x*speed, dir.y*speed, dir.z*speed };

    Vec3 pos = {
        rdseed_to_range(0.0, BOX_SIZE),
        rdseed_to_range(0.0, BOX_SIZE),
        rdseed_to_range(0.0, BOX_SIZE)
    };

    Spike spikes[NUM_SPIKES];
    for (int i = 0; i < NUM_SPIKES; i++) {
        uint64_t ex = entropy::rdseed_xor_jitter();
        uint64_t ey = entropy::rdseed_xor_jitter();
        uint64_t ez = entropy::rdseed_xor_jitter();
        uint64_t er = entropy::rdseed_xor_jitter();
        spikes[i].pos.x  = (double)(ex % (uint64_t)BOX_SIZE);
        spikes[i].pos.y  = (double)(ey % (uint64_t)BOX_SIZE);
        spikes[i].pos.z  = (double)(ez % (uint64_t)BOX_SIZE);
        spikes[i].radius = 1.0 + (double)(er % 8);
    }

    const double DT   = 0.01;
    const double WALL = BOX_SIZE;
    int bounces = 0;

    while (bounces < NUM_BOUNCES) {
        pos.x += vel.x * DT;
        pos.y += vel.y * DT;
        pos.z += vel.z * DT;

        bool bounced = false;
        if (pos.x <= 0.0 || pos.x >= WALL) { vel.x=-vel.x; pos.x=max(0.0,min(WALL,pos.x)); bounced=true; }
        if (pos.y <= 0.0 || pos.y >= WALL) { vel.y=-vel.y; pos.y=max(0.0,min(WALL,pos.y)); bounced=true; }
        if (pos.z <= 0.0 || pos.z >= WALL) { vel.z=-vel.z; pos.z=max(0.0,min(WALL,pos.z)); bounced=true; }

        for (int i = 0; i < NUM_SPIKES; i++) {
            Vec3 diff = sub(pos, spikes[i].pos);
            if (mag(diff) < spikes[i].radius) {
                Vec3 n = norm(diff);
                double d = dot({vel.x,vel.y,vel.z}, n);
                vel.x -= 2.0*d*n.x;
                vel.y -= 2.0*d*n.y;
                vel.z -= 2.0*d*n.z;
                bounced = true;
            }
        }

        if (bounced) result.timestamps[bounces++] = now_ns();
    }

    return result;
}

// ── Visualiser state ─────────────────────────────────────────────────────

SimState init_state() {
    SimState s{};
    s.speed = rdseed_to_range(0.0, 100.0);
    Vec3 dir = {
        rdseed_to_range(-1.0, 1.0),
        rdseed_to_range(-1.0, 1.0),
        rdseed_to_range(-1.0, 1.0)
    };
    dir   = norm(dir);
    s.vel = { dir.x*s.speed, dir.y*s.speed, dir.z*s.speed };
    s.pos = {
        rdseed_to_range(0.0, BOX_SIZE),
        rdseed_to_range(0.0, BOX_SIZE),
        rdseed_to_range(0.0, BOX_SIZE)
    };
    for (int i = 0; i < NUM_SPIKES; i++) {
        uint64_t ex = entropy::rdseed_xor_jitter();
        uint64_t ey = entropy::rdseed_xor_jitter();
        uint64_t ez = entropy::rdseed_xor_jitter();
        uint64_t er = entropy::rdseed_xor_jitter();
        s.spikes[i].pos.x  = (double)(ex % (uint64_t)BOX_SIZE);
        s.spikes[i].pos.y  = (double)(ey % (uint64_t)BOX_SIZE);
        s.spikes[i].pos.z  = (double)(ez % (uint64_t)BOX_SIZE);
        s.spikes[i].radius = 1.0 + (double)(er % 8);
    }
    s.bounce = 0;
    return s;
}

bool tick(SimState& s) {
    const double DT   = 0.05;
    const double WALL = BOX_SIZE;
    bool bounced = false;

    s.pos.x += s.vel.x * DT;
    s.pos.y += s.vel.y * DT;
    s.pos.z += s.vel.z * DT;

    if (s.pos.x <= 0.0 || s.pos.x >= WALL) { s.vel.x=-s.vel.x; s.pos.x=max(0.0,min(WALL,s.pos.x)); bounced=true; }
    if (s.pos.y <= 0.0 || s.pos.y >= WALL) { s.vel.y=-s.vel.y; s.pos.y=max(0.0,min(WALL,s.pos.y)); bounced=true; }
    if (s.pos.z <= 0.0 || s.pos.z >= WALL) { s.vel.z=-s.vel.z; s.pos.z=max(0.0,min(WALL,s.pos.z)); bounced=true; }

    for (int i = 0; i < NUM_SPIKES; i++) {
        Vec3 diff = sub(s.pos, s.spikes[i].pos);
        if (mag(diff) < s.spikes[i].radius) {
            Vec3 n = norm(diff);
            double d = dot({s.vel.x,s.vel.y,s.vel.z}, n);
            s.vel.x -= 2.0*d*n.x;
            s.vel.y -= 2.0*d*n.y;
            s.vel.z -= 2.0*d*n.z;
            bounced = true;
        }
    }

    if (bounced) s.bounce++;
    return bounced;
}

// ── Keccak-256 (SHA-3) ───────────────────────────────────────────────────

static const uint64_t RC[24] = {
    0x0000000000000001ULL,0x0000000000008082ULL,0x800000000000808aULL,
    0x8000000080008000ULL,0x000000000000808bULL,0x0000000080000001ULL,
    0x8000000080008081ULL,0x8000000000008009ULL,0x000000000000008aULL,
    0x0000000000000088ULL,0x0000000080008009ULL,0x000000008000000aULL,
    0x000000008000808bULL,0x800000000000008bULL,0x8000000000008089ULL,
    0x8000000000008003ULL,0x8000000000008002ULL,0x8000000000000080ULL,
    0x000000000000800aULL,0x800000008000000aULL,0x8000000080008081ULL,
    0x8000000000008080ULL,0x0000000080000001ULL,0x8000000080008008ULL
};
static const int RHO[24] = {1,62,28,27,36,44,6,55,20,3,10,43,25,39,41,45,15,21,8,18,2,61,56,14};
static const int PI_[24] = {10,7,11,17,18,3,5,16,8,21,24,4,15,23,19,13,12,2,20,14,22,9,6,1};

static inline uint64_t rotl64(uint64_t x, int n) { return (x<<n)|(x>>(64-n)); }

static void keccak_f(uint64_t st[25]) {
    for (int r=0;r<24;r++) {
        uint64_t bc[5],t;
        for (int i=0;i<5;i++) bc[i]=st[i]^st[i+5]^st[i+10]^st[i+15]^st[i+20];
        for (int i=0;i<5;i++){t=bc[(i+4)%5]^rotl64(bc[(i+1)%5],1);for(int j=0;j<25;j+=5)st[j+i]^=t;}
        uint64_t last=st[1];
        for (int i=0;i<24;i++){int j=PI_[i];t=st[j];st[j]=rotl64(last,RHO[i]);last=t;}
        for (int j=0;j<25;j+=5){uint64_t t0=st[j],t1=st[j+1],t2=st[j+2],t3=st[j+3],t4=st[j+4];
            st[j]^=(~t1)&t2;st[j+1]^=(~t2)&t3;st[j+2]^=(~t3)&t4;st[j+3]^=(~t4)&t0;st[j+4]^=(~t0)&t1;}
        st[0]^=RC[r];
    }
}

static void keccak256(const uint8_t* in, size_t len, uint8_t* out) {
    const size_t rate=136;
    uint64_t st[25]={};
    uint8_t tmp[136];
    while (len>=rate){
        for(size_t i=0;i<rate/8;i++){uint64_t v=0;for(int b=0;b<8;b++)v|=(uint64_t)in[i*8+b]<<(8*b);st[i]^=v;}
        keccak_f(st);in+=rate;len-=rate;
    }
    memset(tmp,0,rate);memcpy(tmp,in,len);
    tmp[len]=0x06;tmp[rate-1]^=0x80;
    for(size_t i=0;i<rate/8;i++){uint64_t v=0;for(int b=0;b<8;b++)v|=(uint64_t)tmp[i*8+b]<<(8*b);st[i]^=v;}
    keccak_f(st);
    for(int i=0;i<4;i++)for(int b=0;b<8;b++)out[i*8+b]=(st[i]>>(8*b))&0xFF;
}

// ── Distillation ─────────────────────────────────────────────────────────

uint64_t distil(const SimResult& result) {
    uint8_t buf[2400];
    memcpy(buf, result.timestamps, 2400);
    uint8_t folded[1200];
    for (int i=0;i<1200;i++) folded[i]=buf[i]^buf[i+1200];
    uint8_t hash[32];
    keccak256(folded,1200,hash);
    uint64_t out=0;
    for (int i=0;i<8;i++) out=(out<<8)|hash[i];
    return out;
}

void velocity_to_bits(double vel, char out[87]) {
    // Not used in visualiser but satisfies linker
    (void)vel; out[0]='\0';
}

} // namespace physics
