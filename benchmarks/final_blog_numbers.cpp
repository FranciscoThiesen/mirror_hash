// Generate accurate numbers for blog post with the new optimized implementation
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>

#include "../include/mirror_hash_unified.hpp"
#include "../third_party/smhasher/rapidhash.h"

using Clock = std::chrono::high_resolution_clock;
volatile uint64_t sink = 0;

template<typename HashFn>
double benchmark(HashFn fn, const void* data, size_t len, size_t iterations) {
    uint64_t warmup = 0;
    for (size_t i = 0; i < 10000; i++) warmup += fn(data, len, i);
    sink = warmup;

    std::vector<double> times;
    for (int run = 0; run < 15; run++) {
        auto start = Clock::now();
        uint64_t sum = 0;
        for (size_t i = 0; i < iterations; i++) sum += fn(data, len, i);
        auto end = Clock::now();
        sink = sum;
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        times.push_back(static_cast<double>(ns) / iterations);
    }
    std::sort(times.begin(), times.end());
    return times[7]; // median of 15
}

int main() {
    printf("=== Final Blog Numbers (mirror_hash v2.1) ===\n\n");

    auto rapid_fn = [](const void* d, size_t l, uint64_t s) -> uint64_t {
        return rapidhash_withSeed(d, l, s);
    };
    auto mirror_fn = [](const void* d, size_t l, uint64_t s) -> uint64_t {
        return mirror_hash::hash(d, l, s);
    };

    // Chart data sizes
    std::vector<size_t> chart_sizes = {8, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 2048, 4096, 8192};

    printf("=== Chart Data (for generate_blog_charts.py) ===\n");
    printf("sizes = [");
    for (size_t i = 0; i < chart_sizes.size(); i++) {
        printf("%zu%s", chart_sizes[i], i < chart_sizes.size()-1 ? ", " : "");
    }
    printf("]\n\n");

    printf("mirror_ns = [");
    for (size_t i = 0; i < chart_sizes.size(); i++) {
        size_t sz = chart_sizes[i];
        std::vector<uint8_t> data(sz);
        std::mt19937_64 rng(12345 + sz);
        for (auto& b : data) b = rng() & 0xFF;

        size_t iters = sz <= 128 ? 10000000 : (sz <= 1024 ? 5000000 : 2000000);
        double t = benchmark(mirror_fn, data.data(), sz, iters);
        printf("%.2f%s", t, i < chart_sizes.size()-1 ? ", " : "");
    }
    printf("]\n\n");

    printf("rapid_ns = [");
    for (size_t i = 0; i < chart_sizes.size(); i++) {
        size_t sz = chart_sizes[i];
        std::vector<uint8_t> data(sz);
        std::mt19937_64 rng(12345 + sz);
        for (auto& b : data) b = rng() & 0xFF;

        size_t iters = sz <= 128 ? 10000000 : (sz <= 1024 ? 5000000 : 2000000);
        double t = benchmark(rapid_fn, data.data(), sz, iters);
        printf("%.2f%s", t, i < chart_sizes.size()-1 ? ", " : "");
    }
    printf("]\n\n");

    // Key sizes for blog table
    printf("=== Key Sizes Table ===\n");
    printf("| Size | rapidhash | mirror_hash | Speedup |\n");
    printf("|------|-----------|-------------|----------|\n");

    std::vector<size_t> key_sizes = {8, 16, 32, 48, 64, 80, 96, 112, 128, 256, 512, 1024, 4096, 8192};
    for (size_t sz : key_sizes) {
        std::vector<uint8_t> data(sz);
        std::mt19937_64 rng(12345 + sz);
        for (auto& b : data) b = rng() & 0xFF;

        size_t iters = sz <= 128 ? 10000000 : (sz <= 1024 ? 5000000 : 2000000);
        double rapid = benchmark(rapid_fn, data.data(), sz, iters);
        double mirror = benchmark(mirror_fn, data.data(), sz, iters);
        double speedup = (rapid / mirror - 1.0) * 100;

        printf("| %zuB | %.2f ns | %.2f ns | %+.0f%% |\n", sz, rapid, mirror, speedup);
    }

    // Transition zone analysis (33-128 bytes)
    printf("\n=== Transition Zone (33-128 bytes) ===\n");
    int mirror_wins = 0, rapid_wins = 0;
    double total_speedup = 0;

    for (size_t sz = 33; sz <= 128; sz++) {
        std::vector<uint8_t> data(sz);
        std::mt19937_64 rng(12345 + sz);
        for (auto& b : data) b = rng() & 0xFF;

        double rapid = benchmark(rapid_fn, data.data(), sz, 5000000);
        double mirror = benchmark(mirror_fn, data.data(), sz, 5000000);
        double speedup = (rapid / mirror - 1.0) * 100;
        total_speedup += speedup;

        if (speedup > 0) mirror_wins++;
        else rapid_wins++;
    }

    printf("mirror_hash wins: %d out of 96 sizes (%.1f%%)\n", mirror_wins, mirror_wins * 100.0 / 96);
    printf("Average speedup: %+.1f%%\n", total_speedup / 96);

    // Small inputs (0-32 bytes) - should be even since we use rapidhashNano
    printf("\n=== Small Inputs (0-32 bytes, uses rapidhashNano) ===\n");
    for (size_t sz : {8, 16, 24, 32}) {
        std::vector<uint8_t> data(sz);
        std::mt19937_64 rng(12345 + sz);
        for (auto& b : data) b = rng() & 0xFF;

        double rapid = benchmark(rapid_fn, data.data(), sz, 10000000);
        double mirror = benchmark(mirror_fn, data.data(), sz, 10000000);
        double speedup = (rapid / mirror - 1.0) * 100;

        printf("%zuB: rapid=%.2f ns, mirror=%.2f ns, diff=%+.1f%%\n", sz, rapid, mirror, speedup);
    }

    return 0;
}
