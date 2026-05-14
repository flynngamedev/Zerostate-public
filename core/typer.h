#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>

// ─── ZeroState Typer ──────────────────────────────────────────────────────
// Terminal output utilities — coloured, formatted, minimal.

namespace typer {

    // ANSI colours
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* DIM = "\033[2m";
    constexpr const char* CYAN = "\033[96m";
    constexpr const char* GREEN = "\033[92m";
    constexpr const char* YELLOW = "\033[93m";
    constexpr const char* RED = "\033[91m";
    constexpr const char* WHITE = "\033[97m";
    constexpr const char* GRAY = "\033[90m";

    inline void banner() {
        printf("\n");
        printf("%s%s  ███████╗███████╗██████╗  ██████╗ %s\n", BOLD, CYAN, RESET);
        printf("%s%s  ╚══███╔╝██╔════╝██╔══██╗██╔═══██╗%s\n", BOLD, CYAN, RESET);
        printf("%s%s    ███╔╝ █████╗  ██████╔╝██║   ██║%s\n", BOLD, CYAN, RESET);
        printf("%s%s   ███╔╝  ██╔══╝  ██╔══██╗██║   ██║%s\n", BOLD, CYAN, RESET);
        printf("%s%s  ███████╗███████╗██║  ██║╚██████╔╝%s\n", BOLD, CYAN, RESET);
        printf("%s%s  ╚══════╝╚══════╝╚═╝  ╚═╝ ╚═════╝ %s\n", BOLD, CYAN, RESET);
        printf("%s%s         S T A T E%s\n\n", BOLD, CYAN, RESET);
        printf("%s  Quantum-Seeded Physics RNG  |  v1.0%s\n", GRAY, RESET);
        printf("%s  True zero is unreachable.%s\n\n", DIM, RESET);
    }

    inline void section(const char* title) {
        printf("\n%s%s-- %s %s", BOLD, CYAN, title, RESET);
        printf("%s--------------------------------%s\n", GRAY, RESET);
    }

    inline void kv(const char* key, const char* fmt, ...) {
        printf("  %s%-18s%s", GRAY, key, RESET);
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }

    inline void kv_u64(const char* key, uint64_t val) {
        printf("  %s%-18s%s%s%llu%s\n", GRAY, key, RESET, WHITE, (unsigned long long)val, RESET);
    }

    inline void kv_f64(const char* key, double val) {
        printf("  %s%-18s%s%s%.6f%s\n", GRAY, key, RESET, WHITE, val, RESET);
    }

    inline void result(uint64_t val) {
        printf("\n  %s%s>  OUTPUT: %llu%s\n\n", BOLD, GREEN, (unsigned long long)val, RESET);
    }

    inline void warn(const char* msg) {
        printf("  %s%s!  %s%s\n", BOLD, YELLOW, msg, RESET);
    }

    inline void ok(const char* msg) {
        printf("  %s%s+  %s%s\n", BOLD, GREEN, msg, RESET);
    }

    inline void divider() {
        printf("%s  ----------------------------------------%s\n", GRAY, RESET);
    }

} // namespace typer