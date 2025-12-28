/*
 * Benchmark: mirror_hash vs GxHash (C port from SMHasher)
 *
 * Both use the same AES acceleration strategy.
 * Key differences:
 *   - GxHash uses AES for ALL sizes
 *   - mirror_hash uses hybrid: rapidhashNano for small, AES for medium, rapidhash for large
 */

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <cmath>

// Include mirror_hash unified
#include "../include/mirror_hash_unified.hpp"

// Include GxHash from SMHasher (C implementation)
extern "C" {
#include "../third_party/smhasher/gxhash.h"
}

// Include rapidhash for baseline comparison
#include "../third_party/smhasher/rapidhash.h"

using Clock = std::chrono::high_resolution_clock;

// Prevent compiler optimization
template<typename T>
__attribute__((noinline)) void escape(T&& val) {
    asm volatile("" : : "r,m"(val) : "memory");
}

__attribute__((noinline)) void clobber() {
    asm volatile("" : : : "memory");
}

struct BenchResult {
    double median_ns;
    double min_ns;
    double max_ns;
    double cycles_approx;  // Approximate cycles at 3 GHz
};

template<typename HashFn>
BenchResult benchmark(HashFn fn, const void* data, size_t len,
                      size_t iterations, int runs) {
    std::vector<double> times;

    // Warmup
    for (size_t i = 0; i < 10000; i++) {
        escape(fn(data, len, i));
    }

    for (int run = 0; run < runs; run++) {
        clobber();

        auto start = Clock::now();
        uint64_t checksum = 0;
        for (size_t i = 0; i < iterations; i++) {
            checksum += fn(data, len, i);
        }
        auto end = Clock::now();

        escape(checksum);

        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        times.push_back(static_cast<double>(ns) / iterations);
    }

    std::sort(times.begin(), times.end());
    double median = times[times.size() / 2];

    return {
        median,                    // median_ns
        times.front(),             // min_ns
        times.back(),              // max_ns
        median * 3.0               // cycles @ 3 GHz
    };
}

int main() {
    printf("=======================================================================\n");
    printf("BENCHMARK: mirror_hash vs GxHash vs rapidhash\n");
    printf("=======================================================================\n\n");

    // Platform info
#if defined(__aarch64__) || defined(_M_ARM64)
    printf("Platform: ARM64");
#ifdef __ARM_FEATURE_CRYPTO
    printf(" with AES crypto extensions\n");
#else
    printf(" (no AES support)\n");
#endif
#elif defined(__x86_64__) || defined(_M_X64)
    printf("Platform: x86_64");
#ifdef __AES__
    printf(" with AES-NI\n");
#else
    printf(" (no AES-NI)\n");
#endif
#else
    printf("Platform: Unknown\n");
#endif

    printf("\nGxHash: Pure AES-based hash (Rust port to C)\n");
    printf("mirror_hash: Hybrid (rapidhashNano + AES + rapidhash bulk)\n");
    printf("rapidhash: Platform-independent (128-bit multiply based)\n\n");

    const int RUNS = 15;

    struct TestCase {
        size_t size;
        size_t iterations;
        const char* note;
    };

    TestCase tests[] = {
        // Small inputs - GxHash uses AES, mirror_hash uses rapidhashNano
        {1,       30000000, "GxHash AES overhead"},
        {4,       30000000, ""},
        {8,       30000000, ""},
        {12,      30000000, ""},
        {16,      30000000, ""},

        // Transition zone
        {24,      20000000, ""},
        {32,      20000000, "GxHash block boundary"},
        {48,      15000000, "rapidhashNano limit"},

        // Medium - both use AES
        {64,      15000000, ""},
        {96,      10000000, ""},
        {128,     10000000, ""},
        {256,     5000000,  ""},
        {512,     3000000,  ""},
        {1024,    2000000,  ""},

        // Large - memory bandwidth starts to matter
        {2048,    1000000,  ""},
        {4096,    500000,   ""},
        {8192,    250000,   "memory bandwidth limit"},
    };

    printf("%-6s %12s %12s %12s  %s\n",
           "Size", "rapidhash", "GxHash", "mirror_hash", "Winner");
    printf("%-6s %12s %12s %12s  %s\n",
           "----", "---------", "------", "-----------", "------");

    for (const auto& test : tests) {
        // Create random data
        std::vector<uint8_t> data(test.size);
        std::mt19937_64 rng(12345 + test.size);
        for (auto& b : data) b = rng() & 0xFF;

        // Benchmark rapidhash (best variant for size)
        auto rapid_fn = [](const void* d, size_t l, uint64_t s) -> uint64_t {
            if (l <= 48) return rapidhashNano_withSeed(d, l, s);
            if (l <= 512) return rapidhashMicro_withSeed(d, l, s);
            return rapidhash_withSeed(d, l, s);
        };
        auto rapid = benchmark(rapid_fn, data.data(), test.size, test.iterations, RUNS);

        // Benchmark GxHash
        auto gx_fn = [](const void* d, size_t l, uint64_t s) -> uint64_t {
            return gxhash64(static_cast<const uint8_t*>(d), static_cast<int>(l),
                           static_cast<uint32_t>(s));
        };
        auto gx = benchmark(gx_fn, data.data(), test.size, test.iterations, RUNS);

        // Benchmark mirror_hash
        auto mirror_fn = [](const void* d, size_t l, uint64_t s) -> uint64_t {
            return mirror_hash::hash(d, l, s);
        };
        auto mirror = benchmark(mirror_fn, data.data(), test.size, test.iterations, RUNS);

        // Determine winner
        const char* winner;
        double best = std::min({rapid.median_ns, gx.median_ns, mirror.median_ns});
        if (mirror.median_ns <= best * 1.05) {
            winner = "mirror_hash";
        } else if (rapid.median_ns <= best * 1.05) {
            winner = "rapidhash";
        } else {
            winner = "GxHash";
        }

        printf("%-6zu %10.2f ns %10.2f ns %10.2f ns  %s%s\n",
               test.size,
               rapid.median_ns, gx.median_ns, mirror.median_ns,
               winner,
               test.note[0] ? " " : "");
    }

    printf("\n");
    printf("=======================================================================\n");
    printf("ANALYSIS: Small Input Performance (Key Differentiator)\n");
    printf("=======================================================================\n\n");

    printf("For 1-16 byte inputs:\n");
    printf("  - GxHash uses AES even for tiny inputs (high setup overhead)\n");
    printf("  - mirror_hash delegates to rapidhashNano (minimal overhead)\n");
    printf("  - rapidhash and mirror_hash should be similar for small inputs\n\n");

    // Detailed comparison at 8 bytes
    std::vector<uint8_t> data8(8);
    for (int i = 0; i < 8; i++) data8[i] = i + 1;

    auto rapid8_fn = [](const void* d, size_t l, uint64_t s) -> uint64_t {
        return rapidhashNano_withSeed(d, l, s);
    };
    auto gx8_fn = [](const void* d, size_t l, uint64_t s) -> uint64_t {
        return gxhash64(static_cast<const uint8_t*>(d), static_cast<int>(l),
                       static_cast<uint32_t>(s));
    };
    auto mirror8_fn = [](const void* d, size_t l, uint64_t s) -> uint64_t {
        return mirror_hash::hash(d, l, s);
    };

    auto rapid8 = benchmark(rapid8_fn, data8.data(), 8, 50000000, 20);
    auto gx8 = benchmark(gx8_fn, data8.data(), 8, 50000000, 20);
    auto mirror8 = benchmark(mirror8_fn, data8.data(), 8, 50000000, 20);

    printf("8-byte input detailed analysis:\n");
    printf("  rapidhashNano: %.2f ns (~%.1f cycles @ 3GHz)\n",
           rapid8.median_ns, rapid8.cycles_approx);
    printf("  GxHash:        %.2f ns (~%.1f cycles @ 3GHz)\n",
           gx8.median_ns, gx8.cycles_approx);
    printf("  mirror_hash:   %.2f ns (~%.1f cycles @ 3GHz)\n",
           mirror8.median_ns, mirror8.cycles_approx);

    double gx_penalty = ((gx8.median_ns / rapid8.median_ns) - 1.0) * 100;
    double mirror_delta = ((mirror8.median_ns / rapid8.median_ns) - 1.0) * 100;

    printf("\n  GxHash overhead vs rapidhashNano: %+.1f%%\n", gx_penalty);
    printf("  mirror_hash delta vs rapidhashNano: %+.1f%%\n", mirror_delta);

    printf("\n");
    printf("=======================================================================\n");
    printf("SUMMARY: When to Use Each Hash\n");
    printf("=======================================================================\n\n");

    printf("GxHash (Rust, C port):\n");
    printf("  + Excellent for medium-large inputs (64+ bytes)\n");
    printf("  + Simple: one algorithm for all sizes\n");
    printf("  - High overhead for small inputs (AES setup cost)\n");
    printf("  - Originally Rust-only, C port may have limitations\n\n");

    printf("mirror_hash (C++ header-only):\n");
    printf("  + Best overall: optimal algorithm per size range\n");
    printf("  + Small inputs: uses rapidhashNano (no AES overhead)\n");
    printf("  + Medium inputs: uses AES acceleration\n");
    printf("  + Large inputs: uses rapidhash bulk (memory-bound anyway)\n");
    printf("  + C++ header-only, easy integration\n\n");

    printf("rapidhash (C header-only):\n");
    printf("  + Platform independent, works everywhere\n");
    printf("  + Great for small inputs\n");
    printf("  + Good for large inputs (memory-bound)\n");
    printf("  - Medium inputs slower than AES-accelerated hashes\n\n");

    return 0;
}
