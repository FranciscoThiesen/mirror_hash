// Simple profiling benchmark - works with standard clang
// For use with macOS sample/Instruments profiling

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <vector>
#include <random>

#include "../include/mirror_hash_unified.hpp"
#include "../third_party/smhasher/rapidhash.h"

using Clock = std::chrono::high_resolution_clock;

__attribute__((noinline)) void escape(uint64_t val) {
    asm volatile("" : : "r"(val) : "memory");
}

__attribute__((noinline)) void clobber() {
    asm volatile("" : : : "memory");
}

// Benchmark function - marked noinline for profiling visibility
__attribute__((noinline))
uint64_t bench_mirror_hash(const void* data, size_t len, size_t iterations) {
    uint64_t sum = 0;
    for (size_t i = 0; i < iterations; i++) {
        sum += mirror_hash::hash(data, len, i);
    }
    return sum;
}

__attribute__((noinline))
uint64_t bench_rapidhash(const void* data, size_t len, size_t iterations) {
    uint64_t sum = 0;
    for (size_t i = 0; i < iterations; i++) {
        if (len <= 48) sum += rapidhashNano_withSeed(data, len, i);
        else if (len <= 512) sum += rapidhashMicro_withSeed(data, len, i);
        else sum += rapidhash_withSeed(data, len, i);
    }
    return sum;
}

int main(int argc, char* argv[]) {
    size_t target_size = 512;  // Default
    size_t iterations = 10000000;

    if (argc > 1) target_size = std::stoull(argv[1]);
    if (argc > 2) iterations = std::stoull(argv[2]);

    printf("Profiling benchmark: %zu bytes, %zu iterations\n", target_size, iterations);
    printf("Use 'sample' command to profile: sample ./profile_benchmark <size> <iters> -f profile.txt\n\n");

    // Generate test data
    std::vector<uint8_t> data(target_size);
    std::mt19937_64 rng(12345);
    for (auto& b : data) b = rng() & 0xFF;

    // Warmup
    printf("Warming up...\n");
    for (size_t i = 0; i < 100000; i++) {
        escape(mirror_hash::hash(data.data(), target_size, i));
        escape(rapidhash_withSeed(data.data(), target_size, i));
    }

    printf("Running mirror_hash benchmark...\n");
    clobber();
    auto start1 = Clock::now();
    uint64_t sum1 = bench_mirror_hash(data.data(), target_size, iterations);
    auto end1 = Clock::now();
    escape(sum1);

    auto ns1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count();
    double ns_per_hash1 = static_cast<double>(ns1) / iterations;

    printf("Running rapidhash benchmark...\n");
    clobber();
    auto start2 = Clock::now();
    uint64_t sum2 = bench_rapidhash(data.data(), target_size, iterations);
    auto end2 = Clock::now();
    escape(sum2);

    auto ns2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2).count();
    double ns_per_hash2 = static_cast<double>(ns2) / iterations;

    printf("\nResults for %zu bytes:\n", target_size);
    printf("  mirror_hash: %.2f ns/hash (%.2f GB/s)\n", ns_per_hash1, target_size / ns_per_hash1);
    printf("  rapidhash:   %.2f ns/hash (%.2f GB/s)\n", ns_per_hash2, target_size / ns_per_hash2);
    printf("  speedup:     %.1f%%\n", (ns_per_hash2 / ns_per_hash1 - 1.0) * 100);

    return 0;
}
