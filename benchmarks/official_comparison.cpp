// ============================================================================
// OFFICIAL HASH FUNCTION COMPARISON
// ============================================================================
// Uses REAL implementations from official repositories:
// - wyhash: https://github.com/wangyi-fudan/wyhash
// - rapidhash: https://github.com/Nicoshev/rapidhash
// - xxHash: https://github.com/Cyan4973/xxHash
// - MurmurHash3: https://github.com/aappleby/smhasher
// ============================================================================

#include "mirror_hash/mirror_hash.hpp"

// Enable xxHash inline implementation (single-file mode)
#define XXH_INLINE_ALL
#include "../third_party/xxhash_official.h"

// Official wyhash
#include "../third_party/wyhash_official.h"

// Official rapidhash
#include "../third_party/rapidhash_official.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <cstring>
#include <array>

// ============================================================================
// Benchmark infrastructure
// ============================================================================

inline void do_not_optimize(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

template<typename F>
double bench(F&& f, size_t iters) {
    size_t sink = 0;
    // Warmup
    for (size_t i = 0; i < iters/10; ++i) {
        auto h = f();
        do_not_optimize(&h);
        sink += h;
    }
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iters; ++i) {
        auto h = f();
        do_not_optimize(&h);
        sink += h;
    }
    auto end = std::chrono::high_resolution_clock::now();
    do_not_optimize(&sink);
    return std::chrono::duration<double, std::nano>(end - start).count() / iters;
}

// ============================================================================
// Avalanche quality test
// ============================================================================

template<typename HashFn>
double test_avalanche(HashFn hash_fn, const void* data, size_t data_size) {
    std::vector<uint8_t> buf(data_size);
    std::memcpy(buf.data(), data, data_size);

    uint64_t base = hash_fn(buf.data(), buf.size());
    double total = 0;
    size_t bits_to_test = std::min(data_size * 8, (size_t)64);

    for (size_t bit = 0; bit < bits_to_test; ++bit) {
        buf[bit / 8] ^= (1 << (bit % 8));
        uint64_t flipped = hash_fn(buf.data(), buf.size());
        total += static_cast<double>(__builtin_popcountll(base ^ flipped)) / 64.0;
        buf[bit / 8] ^= (1 << (bit % 8));
    }
    return total / bits_to_test;
}

// ============================================================================
// Main benchmark
// ============================================================================

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  OFFICIAL HASH FUNCTION COMPARISON                                            ║\n";
    std::cout << "║  Using REAL implementations from official repositories                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Libraries used:\n";
    std::cout << "  - wyhash v4.2: https://github.com/wangyi-fudan/wyhash\n";
    std::cout << "  - rapidhash V3: https://github.com/Nicoshev/rapidhash\n";
    std::cout << "  - xxHash 0.8.3: https://github.com/Cyan4973/xxHash\n";
    std::cout << "  - mirror_hash: Our C++26 reflection-based hash\n\n";

    // Prepare test data
    std::mt19937_64 rng(42);

    alignas(64) std::array<uint8_t, 8> data8;
    alignas(64) std::array<uint8_t, 16> data16;
    alignas(64) std::array<uint8_t, 32> data32;
    alignas(64) std::array<uint8_t, 64> data64;
    alignas(64) std::array<uint8_t, 96> data96;
    alignas(64) std::array<uint8_t, 128> data128;
    alignas(64) std::array<uint8_t, 256> data256;
    alignas(64) std::array<uint8_t, 512> data512;
    alignas(64) std::array<uint8_t, 1024> data1024;

    auto fill = [&](auto& arr) {
        for (auto& b : arr) b = rng() & 0xFF;
    };
    fill(data8); fill(data16); fill(data32); fill(data64);
    fill(data96); fill(data128); fill(data256); fill(data512); fill(data1024);

    constexpr size_t ITERS = 5000000;

    // Define hash function wrappers
    auto wyhash_fn = [](const void* p, size_t len) -> uint64_t {
        return wyhash(p, len, 0, _wyp);
    };

    auto rapidhash_fn = [](const void* p, size_t len) -> uint64_t {
        return rapidhash(p, len);
    };

    auto rapidhash_micro_fn = [](const void* p, size_t len) -> uint64_t {
        return rapidhashMicro(p, len);
    };

    auto xxh64_fn = [](const void* p, size_t len) -> uint64_t {
        return XXH64(p, len, 0);
    };

    auto xxh3_fn = [](const void* p, size_t len) -> uint64_t {
        return XXH3_64bits(p, len);
    };

    auto mirror_fn = [](const void* p, size_t len) -> uint64_t {
        return mirror_hash::detail::hash_bytes<mirror_hash::wyhash_policy>(p, len);
    };

    // Size-specific mirror_hash using compile-time optimization
    #define MIRROR_FIXED(size) \
        [](const void* p) -> uint64_t { \
            return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, size>(p); \
        }

    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  PERFORMANCE BENCHMARK (nanoseconds per hash, lower = better)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(18) << "Size"
              << std::setw(12) << "wyhash"
              << std::setw(12) << "rapidhash"
              << std::setw(12) << "rapid_micro"
              << std::setw(12) << "XXH64"
              << std::setw(12) << "XXH3"
              << std::setw(12) << "mirror"
              << std::setw(12) << "mirror_fix"
              << "\n";
    std::cout << std::string(110, '-') << "\n";

    auto run_size = [&]<size_t SIZE>(const char* label, std::array<uint8_t, SIZE>& data) {

        double t_wyhash = bench([&]{ return wyhash_fn(data.data(), SIZE); }, ITERS);
        double t_rapid = bench([&]{ return rapidhash_fn(data.data(), SIZE); }, ITERS);
        double t_rapid_micro = bench([&]{ return rapidhash_micro_fn(data.data(), SIZE); }, ITERS);
        double t_xxh64 = bench([&]{ return xxh64_fn(data.data(), SIZE); }, ITERS);
        double t_xxh3 = bench([&]{ return xxh3_fn(data.data(), SIZE); }, ITERS);
        double t_mirror = bench([&]{ return mirror_fn(data.data(), SIZE); }, ITERS);
        double t_mirror_fixed = bench([&]{
            return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, SIZE>(data.data());
        }, ITERS);

        // Find winner (excluding mirror for now to see competition)
        double best_other = std::min({t_wyhash, t_rapid, t_rapid_micro, t_xxh64, t_xxh3});

        std::cout << std::setw(18) << label;

        auto print_time = [&](double t, bool is_mirror_fixed = false) {
            if (is_mirror_fixed && t <= best_other) {
                std::cout << "\033[32m" << std::setw(12) << t << "\033[0m";  // Green if mirror wins
            } else if (t == best_other && !is_mirror_fixed) {
                std::cout << "\033[33m" << std::setw(12) << t << "\033[0m";  // Yellow for best competitor
            } else {
                std::cout << std::setw(12) << t;
            }
        };

        print_time(t_wyhash);
        print_time(t_rapid);
        print_time(t_rapid_micro);
        print_time(t_xxh64);
        print_time(t_xxh3);
        print_time(t_mirror);
        print_time(t_mirror_fixed, true);

        // Show speedup/slowdown vs best competitor
        double ratio = t_mirror_fixed / best_other;
        if (ratio < 1.0) {
            std::cout << "  +" << std::setprecision(0) << ((1.0/ratio - 1.0) * 100) << "% faster";
        } else if (ratio > 1.05) {
            std::cout << "  -" << std::setprecision(0) << ((ratio - 1.0) * 100) << "% slower";
        } else {
            std::cout << "  ~same";
        }
        std::cout << std::setprecision(2) << "\n";
    };

    run_size("8 bytes", data8);
    run_size("16 bytes", data16);
    run_size("32 bytes", data32);
    run_size("64 bytes", data64);
    run_size("96 bytes", data96);
    run_size("128 bytes", data128);
    run_size("256 bytes", data256);
    run_size("512 bytes", data512);
    run_size("1024 bytes", data1024);

    // Quality verification
    std::cout << "\n═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  HASH QUALITY - AVALANCHE TEST (ideal = 0.5000)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << std::setprecision(4);
    std::cout << std::setw(18) << "Size"
              << std::setw(12) << "wyhash"
              << std::setw(12) << "rapidhash"
              << std::setw(12) << "rapid_micro"
              << std::setw(12) << "XXH64"
              << std::setw(12) << "XXH3"
              << std::setw(12) << "mirror"
              << "\n";
    std::cout << std::string(90, '-') << "\n";

    auto run_quality = [&]<size_t SIZE>(const char* label, std::array<uint8_t, SIZE>& data) {
        std::cout << std::setw(18) << label
                  << std::setw(12) << test_avalanche(wyhash_fn, data.data(), SIZE)
                  << std::setw(12) << test_avalanche(rapidhash_fn, data.data(), SIZE)
                  << std::setw(12) << test_avalanche(rapidhash_micro_fn, data.data(), SIZE)
                  << std::setw(12) << test_avalanche(xxh64_fn, data.data(), SIZE)
                  << std::setw(12) << test_avalanche(xxh3_fn, data.data(), SIZE)
                  << std::setw(12) << test_avalanche(mirror_fn, data.data(), SIZE)
                  << "\n";
    };

    run_quality("8 bytes", data8);
    run_quality("32 bytes", data32);
    run_quality("96 bytes", data96);
    run_quality("512 bytes", data512);

    // Summary
    std::cout << "\n═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  SUMMARY\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << "mirror_hash advantages:\n";
    std::cout << "  1. C++26 reflection: Automatic hashing of any struct without boilerplate\n";
    std::cout << "  2. Compile-time size optimization: Best performance when size is known\n";
    std::cout << "  3. Quality: Uses proven wyhash mixing for excellent avalanche\n";
    std::cout << "  4. Header-only: Easy integration, no dependencies\n\n";

    std::cout << "Note: Colors indicate:\n";
    std::cout << "  \033[33mYellow\033[0m = Best competitor\n";
    std::cout << "  \033[32mGreen\033[0m  = mirror_hash_fixed beats all competitors\n\n";

    return 0;
}
