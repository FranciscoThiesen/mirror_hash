/*
 * Memory Bandwidth Analysis for mirror_hash
 *
 * This benchmark tests whether large inputs are truly memory-bandwidth limited
 * by comparing:
 * 1. L1 cache performance (same 1KB buffer repeated)
 * 2. L2 cache performance (cycling through 256KB)
 * 3. Main memory performance (cycling through 64MB)
 *
 * If the hypothesis is correct:
 * - L1 cache: AES should be faster (compute limited)
 * - Main memory: Both should be similar (memory limited)
 */

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>

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

struct BenchResult {
    double ns_per_hash;
    double gb_per_sec;
    uint64_t checksum;
};

// Benchmark with data staying in L1 cache (hash same buffer repeatedly)
template<typename HashFn>
BenchResult benchmark_l1_cache(HashFn fn, size_t chunk_size, size_t iterations) {
    // Small buffer that fits in L1 (32-64KB typically)
    std::vector<uint8_t> buffer(chunk_size);
    std::mt19937_64 rng(12345);
    for (auto& b : buffer) b = rng() & 0xFF;

    // Warmup to get buffer into L1
    uint64_t checksum = 0;
    for (size_t i = 0; i < 1000; i++) {
        checksum += fn(buffer.data(), chunk_size, i);
    }
    escape(checksum);

    clobber();
    auto start = Clock::now();

    checksum = 0;
    for (size_t i = 0; i < iterations; i++) {
        checksum += fn(buffer.data(), chunk_size, i);
    }

    auto end = Clock::now();
    escape(checksum);

    double ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double ns_per_hash = ns / iterations;
    double bytes_per_ns = (chunk_size * iterations) / ns;
    double gb_per_sec = bytes_per_ns;  // bytes/ns = GB/s

    return {ns_per_hash, gb_per_sec, checksum};
}

// Benchmark with data streaming from main memory (sequential access through large buffer)
template<typename HashFn>
BenchResult benchmark_main_memory(HashFn fn, size_t chunk_size, size_t total_bytes) {
    // Large buffer that exceeds L3 cache (typically > 8-16MB)
    std::vector<uint8_t> buffer(total_bytes);
    std::mt19937_64 rng(12345);
    for (auto& b : buffer) b = rng() & 0xFF;

    size_t num_chunks = total_bytes / chunk_size;

    // Don't warmup - we want cold cache behavior
    clobber();
    auto start = Clock::now();

    uint64_t checksum = 0;
    for (size_t i = 0; i < num_chunks; i++) {
        checksum += fn(buffer.data() + i * chunk_size, chunk_size, i);
    }

    auto end = Clock::now();
    escape(checksum);

    double ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double ns_per_hash = ns / num_chunks;
    double bytes_per_ns = total_bytes / ns;
    double gb_per_sec = bytes_per_ns;  // bytes/ns = GB/s

    return {ns_per_hash, gb_per_sec, checksum};
}

int main() {
    printf("=======================================================================\n");
    printf("MEMORY BANDWIDTH ANALYSIS: Is large input really memory-limited?\n");
    printf("=======================================================================\n\n");

    // Platform info
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

    // Lambda wrappers
    auto rapidhash_fn = [](const void* d, size_t l, uint64_t s) {
        return rapidhash_withSeed(d, l, s);
    };
    auto mirror_fn = [](const void* d, size_t l, uint64_t s) {
        return mirror_hash::hash(d, l, s);
    };

    size_t test_sizes[] = {256, 512, 1024, 2048, 4096};

    printf("=== TEST 1: L1 CACHE (same buffer, repeated) ===\n");
    printf("Buffer fits in L1 cache. CPU is NOT waiting for memory.\n");
    printf("If AES is truly faster, it should show here.\n\n");

    printf("%-8s %15s %15s %15s %10s\n",
           "Size", "rapidhash", "mirror_hash", "mirror GB/s", "Speedup");
    printf("%-8s %15s %15s %15s %10s\n",
           "----", "---------", "-----------", "-----------", "-------");

    for (size_t sz : test_sizes) {
        size_t iters = 10000000 / (sz / 256);  // More iters for smaller sizes

        auto rapid = benchmark_l1_cache(rapidhash_fn, sz, iters);
        auto mirror = benchmark_l1_cache(mirror_fn, sz, iters);

        double speedup = (rapid.ns_per_hash / mirror.ns_per_hash - 1.0) * 100;

        printf("%-8zu %12.2f ns %12.2f ns %12.2f GB/s %+9.1f%%\n",
               sz, rapid.ns_per_hash, mirror.ns_per_hash, mirror.gb_per_sec, speedup);
    }

    printf("\n=== TEST 2: MAIN MEMORY (streaming, cold cache) ===\n");
    printf("Data streamed from main memory. If memory-limited, both should be similar.\n");
    printf("Total buffer: 64MB (exceeds typical L3 cache)\n\n");

    const size_t TOTAL_BYTES = 64 * 1024 * 1024;  // 64MB

    printf("%-8s %15s %15s %15s %10s\n",
           "Size", "rapidhash", "mirror_hash", "mirror GB/s", "Speedup");
    printf("%-8s %15s %15s %15s %10s\n",
           "----", "---------", "-----------", "-----------", "-------");

    for (size_t sz : test_sizes) {
        auto rapid = benchmark_main_memory(rapidhash_fn, sz, TOTAL_BYTES);
        auto mirror = benchmark_main_memory(mirror_fn, sz, TOTAL_BYTES);

        double speedup = (rapid.ns_per_hash / mirror.ns_per_hash - 1.0) * 100;

        printf("%-8zu %12.2f ns %12.2f ns %12.2f GB/s %+9.1f%%\n",
               sz, rapid.ns_per_hash, mirror.ns_per_hash, mirror.gb_per_sec, speedup);
    }

    printf("\n=== TEST 3: THROUGHPUT COMPARISON ===\n");
    printf("Comparing bytes/second achieved at each cache level.\n\n");

    size_t sz = 1024;  // Use 1KB chunks

    auto l1_rapid = benchmark_l1_cache(rapidhash_fn, sz, 5000000);
    auto l1_mirror = benchmark_l1_cache(mirror_fn, sz, 5000000);
    auto mem_rapid = benchmark_main_memory(rapidhash_fn, sz, TOTAL_BYTES);
    auto mem_mirror = benchmark_main_memory(mirror_fn, sz, TOTAL_BYTES);

    printf("Chunk size: %zu bytes\n\n", sz);
    printf("%-20s %12s %12s\n", "Scenario", "rapidhash", "mirror_hash");
    printf("%-20s %12s %12s\n", "--------", "---------", "-----------");
    printf("%-20s %10.2f GB/s %10.2f GB/s\n", "L1 cache (hot)", l1_rapid.gb_per_sec, l1_mirror.gb_per_sec);
    printf("%-20s %10.2f GB/s %10.2f GB/s\n", "Main memory (cold)", mem_rapid.gb_per_sec, mem_mirror.gb_per_sec);

    printf("\n=== ANALYSIS ===\n");
    double l1_speedup = (l1_rapid.ns_per_hash / l1_mirror.ns_per_hash - 1.0) * 100;
    double mem_speedup = (mem_rapid.ns_per_hash / mem_mirror.ns_per_hash - 1.0) * 100;

    printf("L1 cache speedup:     %+.1f%%\n", l1_speedup);
    printf("Main memory speedup:  %+.1f%%\n", mem_speedup);
    printf("\n");

    if (l1_speedup > 10 && mem_speedup < l1_speedup * 0.5) {
        printf("CONCLUSION: Large inputs ARE memory-bandwidth limited.\n");
        printf("  - L1 cache shows %.1f%% speedup (compute bound)\n", l1_speedup);
        printf("  - Main memory shows only %.1f%% speedup (memory bound)\n", mem_speedup);
        printf("  - The speedup reduction confirms memory bandwidth is the bottleneck.\n");
    } else if (l1_speedup > 10 && mem_speedup > l1_speedup * 0.5) {
        printf("CONCLUSION: Large inputs are NOT fully memory-limited.\n");
        printf("  - L1 shows %.1f%% speedup, main memory shows %.1f%% speedup\n", l1_speedup, mem_speedup);
        printf("  - Consider extending AES optimization to larger inputs.\n");
    } else {
        printf("CONCLUSION: No significant compute speedup observed.\n");
        printf("  - This may be expected on non-ARM64 or non-AES platforms.\n");
    }

    return 0;
}
