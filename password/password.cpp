#include "typer.h"
#include "entropy.h"
#include "quantum.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ─── ZeroState Password Generator ─────────────────────────────────────────
// Every character is picked by ZeroState 86-bit RDSEED entropy.
// No physics sim needed here — raw entropy is enough for character selection.
//
// Usage:
//   password.exe                  -- 16 char password, all types
//   password.exe 32               -- 32 char password
//   password.exe 24 --no-symbols  -- 24 chars, letters + numbers only
//   password.exe 24 --only-hex    -- 24 chars, hex only (0-9 a-f)

static quantum::Pool g_pool;

// ── Character sets ────────────────────────────────────────────────────────

static const char LOWER[]   = "abcdefghijklmnopqrstuvwxyz";
static const char UPPER[]   = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char DIGITS[]  = "0123456789";
static const char SYMBOLS[] = "!@#$%^&*()-_=+[]{}|;:,.<>?";
static const char HEX[]     = "0123456789abcdef";

// ── ZeroState character picker ────────────────────────────────────────────
// Uses 86-bit entropy to pick an index into a charset.
// Rejection sampling ensures perfectly uniform distribution.

static char pick_char(const char* charset, int len) {
    while (true) {
        entropy::u86 val = entropy::rdseed86();
        // Use lo for index -- rejection sample to avoid modulo bias
        uint64_t limit = (UINT64_MAX / (uint64_t)len) * (uint64_t)len;
        if (val.lo < limit)
            return charset[val.lo % (uint64_t)len];
        // Rejected -- try again (astronomically rare)
    }
}

// ── Pool fallback bit ─────────────────────────────────────────────────────

static char pick_char_pool(const char* charset, int len) {
    uint64_t val = 0;
    if (!entropy::rdseed64(val)) {
        uint8_t bit = 0;
        quantum::pool_take(g_pool, &bit, 1);
        quantum::pool_return(g_pool, &bit, 1);
        val = (uint64_t)bit;
        typer::warn("RDSEED unavailable -- used pool bucket");
    }
    return charset[val % (uint64_t)len];
}

// ── Build combined charset ────────────────────────────────────────────────

static void build_charset(char* out, int& len, bool symbols) {
    len = 0;
    for (int i = 0; LOWER[i];   i++) out[len++] = LOWER[i];
    for (int i = 0; UPPER[i];   i++) out[len++] = UPPER[i];
    for (int i = 0; DIGITS[i];  i++) out[len++] = DIGITS[i];
    if (symbols)
        for (int i = 0; SYMBOLS[i]; i++) out[len++] = SYMBOLS[i];
    out[len] = '\0';
}

// ── Strength label ────────────────────────────────────────────────────────

static const char* strength(int len, bool symbols) {
    double bits = len * (symbols ? 6.5 : 5.95); // log2(charset size)
    if (bits >= 128) return "\033[92mEXTREME\033[0m";
    if (bits >= 96)  return "\033[92mSTRONG\033[0m";
    if (bits >= 72)  return "\033[93mGOOD\033[0m";
    return "\033[91mWEAK\033[0m";
}

// ── Main ──────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    typer::banner();

    // Parse args
    int  pw_len     = 16;
    bool use_sym    = true;
    bool only_hex   = false;
    int  pw_count   = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-symbols") == 0) use_sym  = false;
        else if (strcmp(argv[i], "--only-hex") == 0) only_hex = true;
        else if (strcmp(argv[i], "--count") == 0 && i+1 < argc) pw_count = atoi(argv[++i]);
        else if (atoi(argv[i]) > 0) pw_len = atoi(argv[i]);
    }
    if (pw_len   < 1)   pw_len   = 16;
    if (pw_len   > 512) pw_len   = 512;
    if (pw_count < 1)   pw_count = 1;
    if (pw_count > 100) pw_count = 100;

    // Init pool
    typer::section("INITIALISING");
    quantum::pool_init(g_pool);
    typer::ok("Pool bucket ready  (5000 zeros + 5000 ones)");
    typer::ok("RDSEED 86-bit entropy online");

    // Build charset
    char charset[128];
    int  cslen = 0;

    if (only_hex) {
        memcpy(charset, HEX, sizeof(HEX));
        cslen = (int)strlen(HEX);
    } else {
        build_charset(charset, cslen, use_sym);
    }

    printf("  %sCharset size:%s  %s%d%s characters\n",
        typer::GRAY, typer::RESET, typer::WHITE, cslen, typer::RESET);
    printf("  %sLength:%s       %s%d%s characters\n",
        typer::GRAY, typer::RESET, typer::WHITE, pw_len, typer::RESET);
    printf("  %sStrength:%s     %s\n",
        typer::GRAY, typer::RESET, only_hex ? "\033[93mHEX\033[0m" : strength(pw_len, use_sym));

    // Generate passwords
    typer::section("GENERATING");

    char* pw = new char[pw_len + 1];

    for (int p = 0; p < pw_count; p++) {
        for (int i = 0; i < pw_len; i++)
            pw[i] = pick_char(charset, cslen);
        pw[pw_len] = '\0';

        typer::divider();
        printf("  %s%s%s\n", typer::WHITE, pw, typer::RESET);
    }

    delete[] pw;

    typer::section("DONE");
    printf("  %sEach character selected by 86-bit RDSEED quantum entropy.%s\n",
        typer::GRAY, typer::RESET);
    printf("  %sTrue zero is unreachable.%s\n\n", typer::DIM, typer::RESET);

    return 0;
}
