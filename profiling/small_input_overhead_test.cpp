/*
 * Small Input Overhead Analysis
 *
 * This benchmark measures the setup overhead of AES-based hashing vs rapidhashNano
 * for small inputs (0-16 bytes) where setup cost dominates.
 */

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <vector>

#include "../third_party/smhasher/rapidhash.h"
#include "../include/mirror_hash_unified.hpp"

using Clock = std::chrono::high_resolution_clock;

// Prevent optimization
template<typename T>
__attribute__((noinline)) void escape(T&& val) {
    asm volatile("" : : "r,m"(val) : "memory");
}

__attribute__((noinline)) void clobber() {
    asm volatile("" : : : "memory");
}

// Time a single hash call very precisely (many iterations)
template<typename HashFn>
double benchmark_single_size(HashFn fn, const void* data, size_t len, size_t iterations) {
    // Warmup
    uint64_t checksum = 0;
    for (size_t i = 0; i < 1000; i++) {
        checksum += fn(data, len, i);
    }
    escape(checksum);

    clobber();
    auto start = Clock::now();

    checksum = 0;
    for (size_t i = 0; i < iterations; i++) {
        checksum += fn(data, len, i);
    }

    auto end = Clock::now();
    escape(checksum);

    double ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    return ns / iterations;
}

#ifdef MIRROR_HASH_HAS_AES
// Direct AES hash for comparison (bypass the size check)
__attribute__((noinline))
uint64_t force_aes_hash(const void* data, size_t len, uint64_t seed) {
    return mirror_hash::detail::hash_aes(data, len, seed);
}
#endif

int main() {
    printf("=======================================================================\n");
    printf("SMALL INPUT OVERHEAD ANALYSIS: Why can't we beat rapidhashNano?\n");
    printf("=======================================================================\n\n");

#if defined(__aarch64__) || defined(_M_ARM64)
    printf("Platform: ARM64");
#ifdef MIRROR_HASH_HAS_AES
    printf(" with AES crypto extensions\n");
#else
    printf(" (no AES support)\n");
#endif
#elif defined(__x86_64__) || defined(_M_X64)
    printf("Platform: x86_64\n");
#endif
    printf("mirror_hash AES: %s\n\n", mirror_hash::has_aes() ? "YES" : "NO");

    // Test data - aligned for optimal access
    alignas(16) char data[32] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};

    auto rapidhash_nano_fn = [](const void* d, size_t l, uint64_t s) {
        return rapidhashNano_withSeed(d, l, s);
    };

    auto mirror_fn = [](const void* d, size_t l, uint64_t s) {
        return mirror_hash::hash(d, l, s);
    };

    const size_t iterations = 10000000;

    printf("=== PART 1: Small inputs where rapidhashNano wins ===\n\n");
    printf("%-8s %15s %15s %10s\n", "Size", "rapidhashNano", "mirror_hash", "Speedup");
    printf("%-8s %15s %15s %10s\n", "----", "-------------", "-----------", "-------");

    for (size_t sz = 1; sz <= 16; sz++) {
        double rapid_ns = benchmark_single_size(rapidhash_nano_fn, data, sz, iterations);
        double mirror_ns = benchmark_single_size(mirror_fn, data, sz, iterations);
        double speedup = (rapid_ns / mirror_ns - 1.0) * 100;

        printf("%-8zu %12.2f ns %12.2f ns %+9.1f%%\n", sz, rapid_ns, mirror_ns, speedup);
    }

#ifdef MIRROR_HASH_HAS_AES
    printf("\n=== PART 2: What if we FORCED AES for small inputs? ===\n");
    printf("This shows the overhead penalty of using AES for small inputs.\n\n");

    auto force_aes_fn = [](const void* d, size_t l, uint64_t s) {
        return force_aes_hash(d, l, s);
    };

    printf("%-8s %15s %15s %10s\n", "Size", "rapidhashNano", "force_AES", "AES Penalty");
    printf("%-8s %15s %15s %10s\n", "----", "-------------", "---------", "-----------");

    for (size_t sz = 1; sz <= 16; sz++) {
        double rapid_ns = benchmark_single_size(rapidhash_nano_fn, data, sz, iterations);
        double aes_ns = benchmark_single_size(force_aes_fn, data, sz, iterations);
        double penalty = (aes_ns / rapid_ns - 1.0) * 100;

        printf("%-8zu %12.2f ns %12.2f ns %+9.1f%%\n", sz, rapid_ns, aes_ns, penalty);
    }

    printf("\n=== PART 3: Crossover point analysis ===\n");
    printf("Finding where AES starts to win.\n\n");

    printf("%-8s %15s %15s %10s\n", "Size", "rapidhashNano", "AES", "Speedup");
    printf("%-8s %15s %15s %10s\n", "----", "-------------", "---", "-------");

    for (size_t sz = 8; sz <= 64; sz += 4) {
        double rapid_ns = benchmark_single_size(rapidhash_nano_fn, data, sz, iterations);
        double aes_ns = benchmark_single_size(force_aes_fn, data, sz, iterations);
        double speedup = (rapid_ns / aes_ns - 1.0) * 100;

        const char* winner = speedup > 0 ? "AES wins" : "Nano wins";
        printf("%-8zu %12.2f ns %12.2f ns %+9.1f%% %s\n", sz, rapid_ns, aes_ns, speedup, winner);
    }
#endif

    printf("\n=== ANALYSIS ===\n");
    printf("For inputs ≤16 bytes:\n");
    printf("  - rapidhashNano uses only 3 multiplies with minimal setup\n");
    printf("  - AES requires loading 4 round keys + buffer handling + 4 AES rounds\n");
    printf("  - The setup overhead exceeds any computational advantage\n");
    printf("\nThis is why mirror_hash delegates to rapidhashNano for small inputs.\n");

    return 0;
}
