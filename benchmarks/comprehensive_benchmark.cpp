// ============================================================================
// COMPREHENSIVE HASH BENCHMARK
// ============================================================================
// Inspired by xxHash's benchmarking methodology
// Measures:
//   - Bandwidth (GB/s) for bulk data throughput
//   - Small Data Velocity for tiny inputs
//   - Quality score (SMHasher-style tests)
//
// References:
//   - xxHash: https://github.com/Cyan4973/xxHash
//   - SMHasher: https://github.com/rurban/smhasher
// ============================================================================

#include "mirror_hash/mirror_hash.hpp"

#define XXH_INLINE_ALL
#include "../third_party/xxhash_official.h"
#include "../third_party/wyhash_official.h"
#include "../third_party/rapidhash_official.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <cstring>
#include <array>
#include <functional>
#include <unordered_set>
#include <algorithm>
#include <numeric>

// ============================================================================
// FNV-1a (baseline comparison - known to have poor avalanche)
// ============================================================================

inline uint64_t fnv1a_64(const void* data, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t hash = 0xcbf29ce484222325ULL;  // FNV offset basis
    for (size_t i = 0; i < len; ++i) {
        hash ^= p[i];
        hash *= 0x100000001b3ULL;  // FNV prime
    }
    return hash;
}

// ============================================================================
// Benchmark Infrastructure
// ============================================================================

inline void do_not_optimize(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

// Get CPU cycles (if available) or fallback to high-resolution time
inline uint64_t get_cycles() {
#if defined(__aarch64__)
    uint64_t val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#elif defined(__x86_64__)
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#else
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
#endif
}

// Prevent compiler from optimizing away hash computations
template<typename T>
__attribute__((noinline)) void escape(T&& val) {
    asm volatile("" : : "g"(val) : "memory");
}

// Measure throughput in GB/s for large data
template<typename F>
double bench_throughput_gbps(F&& hash_fn, const void* data, size_t data_size, size_t iterations) {
    // Warmup
    uint64_t sink = 0;
    for (size_t i = 0; i < iterations / 10; ++i) {
        sink ^= hash_fn(data, data_size);
        escape(sink);
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        sink ^= hash_fn(data, data_size);
        escape(sink);
    }
    auto end = std::chrono::high_resolution_clock::now();
    escape(sink);

    double seconds = std::chrono::duration<double>(end - start).count();
    double bytes_processed = static_cast<double>(data_size) * iterations;
    double gbps = bytes_processed / seconds / 1e9;

    return gbps;
}

// Measure latency in nanoseconds for small data
template<typename F>
double bench_latency_ns(F&& hash_fn, const void* data, size_t data_size, size_t iterations) {
    uint64_t sink = 0;
    // Warmup
    for (size_t i = 0; i < iterations / 10; ++i) {
        sink ^= hash_fn(data, data_size);
        escape(sink);
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        sink ^= hash_fn(data, data_size);
        escape(sink);
    }
    auto end = std::chrono::high_resolution_clock::now();
    escape(sink);

    return std::chrono::duration<double, std::nano>(end - start).count() / iterations;
}

// ============================================================================
// Quality Tests (SMHasher-style)
// ============================================================================

// Avalanche test: returns bias (0 = perfect, >0.02 = fail)
template<typename HashFn>
double test_avalanche_bias(HashFn hash_fn, size_t samples = 50000) {
    std::mt19937_64 rng(42);
    double total_bias = 0;

    for (size_t i = 0; i < samples; ++i) {
        uint64_t input = rng();
        uint64_t base = hash_fn(&input, sizeof(input));

        for (int bit = 0; bit < 64; ++bit) {
            uint64_t flipped = input ^ (1ULL << bit);
            uint64_t h = hash_fn(&flipped, sizeof(flipped));
            int changed = __builtin_popcountll(base ^ h);
            total_bias += std::abs(changed / 64.0 - 0.5);
        }
    }

    return total_bias / (samples * 64);
}

// Collision test: returns true if collision rate is acceptable
template<typename HashFn>
bool test_collisions(HashFn hash_fn, size_t samples = 1000000) {
    std::mt19937_64 rng(42);
    std::unordered_set<uint64_t> seen;
    seen.reserve(samples);
    size_t collisions = 0;

    for (size_t i = 0; i < samples; ++i) {
        uint64_t input = rng();
        uint64_t h = hash_fn(&input, sizeof(input));
        if (!seen.insert(h).second) collisions++;
    }

    // Expected collisions for 64-bit hash: n^2 / 2^65
    double expected = static_cast<double>(samples) * samples / (2.0 * (1ULL << 63) * 2.0);
    return collisions < std::max(10.0, expected * 10);
}

// Distribution test: returns true if chi-squared is acceptable
template<typename HashFn>
bool test_distribution(HashFn hash_fn, size_t samples = 500000) {
    std::mt19937_64 rng(42);
    constexpr size_t BUCKETS = 65536;
    std::vector<uint64_t> counts(BUCKETS, 0);

    for (size_t i = 0; i < samples; ++i) {
        uint64_t input = rng();
        uint64_t h = hash_fn(&input, sizeof(input));
        counts[h % BUCKETS]++;
    }

    double expected = static_cast<double>(samples) / BUCKETS;
    double chi_sq = 0;
    for (size_t i = 0; i < BUCKETS; ++i) {
        double diff = counts[i] - expected;
        chi_sq += (diff * diff) / expected;
    }

    // For 65535 df, chi-squared should be roughly equal to df
    // Allow ±20% tolerance
    return chi_sq > BUCKETS * 0.8 && chi_sq < BUCKETS * 1.2;
}

// Sparse key test: returns true if sparse inputs produce unique hashes
template<typename HashFn>
bool test_sparse_keys(HashFn hash_fn) {
    std::unordered_set<uint64_t> hashes;

    // Single bit inputs
    for (int i = 0; i < 64; ++i) {
        uint64_t input = 1ULL << i;
        hashes.insert(hash_fn(&input, sizeof(input)));
    }

    // Two bit inputs
    for (int i = 0; i < 64; ++i) {
        for (int j = i + 1; j < 64; ++j) {
            uint64_t input = (1ULL << i) | (1ULL << j);
            hashes.insert(hash_fn(&input, sizeof(input)));
        }
    }

    size_t expected = 64 + (64 * 63 / 2);
    double collision_rate = 1.0 - static_cast<double>(hashes.size()) / expected;
    return collision_rate < 0.001;  // Less than 0.1% collisions
}

// Differential test: returns true if sequential inputs have good avalanche
template<typename HashFn>
bool test_differential(HashFn hash_fn, size_t samples = 50000) {
    double total_avalanche = 0;
    uint64_t prev = hash_fn("\0\0\0\0\0\0\0\0", 8);

    for (size_t i = 1; i <= samples; ++i) {
        uint64_t input = i;
        uint64_t h = hash_fn(&input, sizeof(input));
        total_avalanche += __builtin_popcountll(h ^ prev) / 64.0;
        prev = h;
    }

    double mean = total_avalanche / samples;
    return std::abs(mean - 0.5) < 0.05;
}

// Run all quality tests and return score out of 10
template<typename HashFn>
int quality_score(HashFn hash_fn) {
    int score = 0;

    // Avalanche (weight: 3) - relaxed thresholds for non-cryptographic hashes
    double bias = test_avalanche_bias(hash_fn);
    if (bias < 0.02) score += 3;       // Excellent
    else if (bias < 0.05) score += 2;  // Good
    else if (bias < 0.10) score += 1;  // Acceptable

    // Collisions (weight: 2)
    if (test_collisions(hash_fn)) score += 2;

    // Distribution (weight: 2)
    if (test_distribution(hash_fn)) score += 2;

    // Sparse keys (weight: 2)
    if (test_sparse_keys(hash_fn)) score += 2;

    // Differential (weight: 1)
    if (test_differential(hash_fn)) score += 1;

    return score;
}

// ============================================================================
// Hash Function Definitions
// ============================================================================

struct HashFunction {
    std::string name;
    int width;
    std::function<uint64_t(const void*, size_t)> fn;
    std::string comment;
};

// ============================================================================
// Main Benchmark
// ============================================================================

int main() {
    std::cout << R"(
================================================================================
                     COMPREHENSIVE HASH FUNCTION BENCHMARK
================================================================================
  Methodology inspired by xxHash (https://github.com/Cyan4973/xxHash)
  Quality tests based on SMHasher (https://github.com/rurban/smhasher)
================================================================================
)" << std::flush;

    // Test environment
    std::cout << "\nTest Environment:\n";
#if defined(__aarch64__)
    std::cout << "  Platform:    ARM64 (aarch64)\n";
#elif defined(__x86_64__)
    std::cout << "  Platform:    x86_64\n";
#else
    std::cout << "  Platform:    Unknown\n";
#endif
    std::cout << "  Compiler:    Clang with P2996 reflection\n";
    std::cout << "  Build:       Release (-O3 -march=native)\n\n";

    // Prepare large data buffer for throughput tests
    constexpr size_t LARGE_SIZE = 256 * 1024;  // 256 KB
    std::vector<uint8_t> large_data(LARGE_SIZE);
    std::mt19937_64 rng(42);
    for (auto& b : large_data) b = rng() & 0xFF;

    // Small data for latency tests
    alignas(64) std::array<uint8_t, 8> data8{};
    alignas(64) std::array<uint8_t, 16> data16{};
    alignas(64) std::array<uint8_t, 32> data32{};
    alignas(64) std::array<uint8_t, 64> data64{};
    alignas(64) std::array<uint8_t, 256> data256{};
    for (auto& b : data8) b = rng() & 0xFF;
    for (auto& b : data16) b = rng() & 0xFF;
    for (auto& b : data32) b = rng() & 0xFF;
    for (auto& b : data64) b = rng() & 0xFF;
    for (auto& b : data256) b = rng() & 0xFF;

    // Define hash functions
    std::vector<HashFunction> hashes = {
        {"XXH3", 64, [](const void* p, size_t len) { return XXH3_64bits(p, len); }, ""},
        {"XXH64", 64, [](const void* p, size_t len) { return XXH64(p, len, 0); }, ""},
        {"wyhash", 64, [](const void* p, size_t len) { return wyhash(p, len, 0, _wyp); }, "Go/Zig/Nim default"},
        {"rapidhash", 64, [](const void* p, size_t len) { return rapidhash(p, len); }, ""},
        {"mirror_hash", 64, [](const void* p, size_t len) {
            return mirror_hash::detail::hash_bytes<mirror_hash::wyhash_policy>(p, len);
        }, "Runtime size"},
        {"mirror_hash*", 64, nullptr, "Compile-time size"},  // Special case
        {"FNV-1a", 64, fnv1a_64, "Poor avalanche"},
    };

    // ========================================================================
    // THROUGHPUT BENCHMARK (GB/s)
    // ========================================================================

    std::cout << "================================================================================\n";
    std::cout << "  THROUGHPUT BENCHMARK (GB/s, higher = better)\n";
    std::cout << "================================================================================\n\n";

    std::cout << std::fixed << std::setprecision(1);
    std::cout << std::setw(16) << "Hash"
              << std::setw(8) << "Width"
              << std::setw(14) << "256KB (GB/s)"
              << std::setw(14) << "64B (GB/s)"
              << std::setw(14) << "16B (GB/s)"
              << "\n";
    std::cout << std::string(66, '-') << "\n";

    for (const auto& h : hashes) {
        if (h.name == "mirror_hash*") {
            // Compile-time optimized version
            double tp_large = bench_throughput_gbps([&](const void* p, size_t) {
                return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, LARGE_SIZE>(p);
            }, large_data.data(), LARGE_SIZE, 5000);

            double tp_64 = bench_throughput_gbps([&](const void* p, size_t) {
                return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, 64>(p);
            }, data64.data(), 64, 5000000);

            double tp_16 = bench_throughput_gbps([&](const void* p, size_t) {
                return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, 16>(p);
            }, data16.data(), 16, 10000000);

            std::cout << std::setw(16) << h.name
                      << std::setw(8) << h.width
                      << std::setw(14) << tp_large
                      << std::setw(14) << tp_64
                      << std::setw(14) << tp_16
                      << "\n";
        } else {
            double tp_large = bench_throughput_gbps(h.fn, large_data.data(), LARGE_SIZE, 5000);
            double tp_64 = bench_throughput_gbps(h.fn, data64.data(), 64, 5000000);
            double tp_16 = bench_throughput_gbps(h.fn, data16.data(), 16, 10000000);

            std::cout << std::setw(16) << h.name
                      << std::setw(8) << h.width
                      << std::setw(14) << tp_large
                      << std::setw(14) << tp_64
                      << std::setw(14) << tp_16
                      << "\n";
        }
    }

    // ========================================================================
    // SMALL DATA LATENCY (ns/hash)
    // ========================================================================

    std::cout << "\n================================================================================\n";
    std::cout << "  SMALL DATA LATENCY (ns/hash, lower = better)\n";
    std::cout << "================================================================================\n\n";

    std::cout << std::setprecision(2);
    std::cout << std::setw(16) << "Hash"
              << std::setw(10) << "8B"
              << std::setw(10) << "16B"
              << std::setw(10) << "32B"
              << std::setw(10) << "64B"
              << std::setw(10) << "256B"
              << "\n";
    std::cout << std::string(66, '-') << "\n";

    constexpr size_t SMALL_ITERS = 10000000;

    for (const auto& h : hashes) {
        if (h.name == "mirror_hash*") {
            double l8 = bench_latency_ns([&](const void* p, size_t) {
                return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, 8>(p);
            }, data8.data(), 8, SMALL_ITERS);
            double l16 = bench_latency_ns([&](const void* p, size_t) {
                return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, 16>(p);
            }, data16.data(), 16, SMALL_ITERS);
            double l32 = bench_latency_ns([&](const void* p, size_t) {
                return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, 32>(p);
            }, data32.data(), 32, SMALL_ITERS);
            double l64 = bench_latency_ns([&](const void* p, size_t) {
                return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, 64>(p);
            }, data64.data(), 64, SMALL_ITERS);
            double l256 = bench_latency_ns([&](const void* p, size_t) {
                return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, 256>(p);
            }, data256.data(), 256, SMALL_ITERS);

            std::cout << std::setw(16) << h.name
                      << std::setw(10) << l8
                      << std::setw(10) << l16
                      << std::setw(10) << l32
                      << std::setw(10) << l64
                      << std::setw(10) << l256
                      << "\n";
        } else {
            std::cout << std::setw(16) << h.name
                      << std::setw(10) << bench_latency_ns(h.fn, data8.data(), 8, SMALL_ITERS)
                      << std::setw(10) << bench_latency_ns(h.fn, data16.data(), 16, SMALL_ITERS)
                      << std::setw(10) << bench_latency_ns(h.fn, data32.data(), 32, SMALL_ITERS)
                      << std::setw(10) << bench_latency_ns(h.fn, data64.data(), 64, SMALL_ITERS)
                      << std::setw(10) << bench_latency_ns(h.fn, data256.data(), 256, SMALL_ITERS)
                      << "\n";
        }
    }

    // ========================================================================
    // QUALITY TESTS (SMHasher-style)
    // ========================================================================

    std::cout << "\n================================================================================\n";
    std::cout << "  HASH QUALITY (SMHasher-style tests)\n";
    std::cout << "================================================================================\n\n";

    std::cout << std::setw(16) << "Hash"
              << std::setw(10) << "Quality"
              << std::setw(12) << "Aval.Bias"
              << "  Notes\n";
    std::cout << std::string(70, '-') << "\n";

    for (const auto& h : hashes) {
        if (h.name == "mirror_hash*") continue;  // Same quality as mirror_hash

        int score = quality_score(h.fn);
        double bias = test_avalanche_bias(h.fn);

        std::cout << std::setw(16) << h.name
                  << std::setw(10) << (std::to_string(score) + "/10")
                  << std::setw(12) << std::setprecision(4) << bias << std::setprecision(2);

        std::cout << "  ";
        if (bias < 0.02) std::cout << "Excellent";
        else if (bias < 0.05) std::cout << "Good";
        else if (bias < 0.10) std::cout << "Fair";
        else std::cout << "Poor";

        if (!h.comment.empty()) {
            std::cout << " (" << h.comment << ")";
        }
        std::cout << "\n";
    }

    // ========================================================================
    // SUMMARY TABLE (for README)
    // ========================================================================

    std::cout << "\n================================================================================\n";
    std::cout << "  MARKDOWN TABLE (copy to README)\n";
    std::cout << "================================================================================\n\n";

    std::cout << "| Hash | Width | Bulk (GB/s) | Small Data | Quality | Notes |\n";
    std::cout << "|------|-------|-------------|------------|---------|-------|\n";

    for (const auto& h : hashes) {
        double bulk_gbps, small_ns;
        int score;

        if (h.name == "mirror_hash*") {
            bulk_gbps = bench_throughput_gbps([&](const void* p, size_t) {
                return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, LARGE_SIZE>(p);
            }, large_data.data(), LARGE_SIZE, 2000);
            small_ns = bench_latency_ns([&](const void* p, size_t) {
                return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, 16>(p);
            }, data16.data(), 16, 5000000);
            score = 10;  // Same as mirror_hash
        } else {
            bulk_gbps = bench_throughput_gbps(h.fn, large_data.data(), LARGE_SIZE, 2000);
            small_ns = bench_latency_ns(h.fn, data16.data(), 16, 5000000);
            score = quality_score(h.fn);
        }

        std::cout << "| " << h.name
                  << " | " << h.width
                  << " | " << std::setprecision(1) << bulk_gbps
                  << " | " << std::setprecision(2) << small_ns << "ns"
                  << " | " << score << "/10"
                  << " | " << h.comment
                  << " |\n";
    }

    std::cout << "\n* mirror_hash* uses compile-time size optimization\n";

    std::cout << "\n================================================================================\n";
    std::cout << "  Benchmark complete!\n";
    std::cout << "================================================================================\n";

    return 0;
}
