// ============================================================================
// MICRO VARIANT REFINED TEST
// ============================================================================
// Focus on tiny inputs (1-8 bytes) where there may be optimization potential
// ============================================================================

#include "mirror_hash/mirror_hash.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <cstring>
#include <array>

inline void do_not_optimize(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

template<typename F>
double bench(F&& f, size_t iters) {
    size_t sink = 0;
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

template<typename HashFn>
double test_avalanche(HashFn hash_fn, size_t data_size, size_t num_tests = 10000) {
    std::mt19937_64 rng(42);
    double total = 0;
    size_t bits_to_test = std::min(data_size * 8, (size_t)64);

    for (size_t t = 0; t < num_tests; ++t) {
        std::vector<uint8_t> buf(data_size);
        for (auto& b : buf) b = rng() & 0xFF;

        uint64_t base = hash_fn(buf.data(), buf.size());

        for (size_t bit = 0; bit < bits_to_test; ++bit) {
            buf[bit / 8] ^= (1 << (bit % 8));
            uint64_t flipped = hash_fn(buf.data(), buf.size());
            total += static_cast<double>(__builtin_popcountll(base ^ flipped)) / 64.0;
            buf[bit / 8] ^= (1 << (bit % 8));
        }
    }
    return total / (num_tests * bits_to_test);
}

// ============================================================================
// MICRO VARIANTS
// ============================================================================

namespace micro {

// Variant A: Current baseline (for reference)
template<size_t N>
uint64_t baseline(const void* data) noexcept {
    return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, N>(data);
}

// Variant B: Simplified with proper mixing (wyhash-compatible quality)
template<size_t N>
[[gnu::always_inline]] inline uint64_t variant_b(const void* data) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);
    constexpr uint64_t wyp0 = 0xa0761d6478bd642full;
    constexpr uint64_t wyp1 = 0xe7037ed1a0b428dbull;
    constexpr uint64_t INIT = 0x1ff5c2923a788d2cull;

    auto wymix = [](uint64_t a, uint64_t b) noexcept -> uint64_t {
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    };

    if constexpr (N == 0) {
        return INIT;
    }
    else if constexpr (N <= 3) {
        // wyhash's 1-3 byte trick: spread bytes across 64-bit value
        uint64_t a = (static_cast<uint64_t>(p[0]) << 16) |
                     (static_cast<uint64_t>(p[N >> 1]) << 8) |
                     static_cast<uint64_t>(p[N - 1]);
        return wymix(a ^ wyp0 ^ N, INIT ^ wyp1);
    }
    else if constexpr (N == 4) {
        uint32_t v;
        __builtin_memcpy(&v, p, 4);
        return wymix(v ^ wyp0 ^ N, INIT ^ wyp1);
    }
    else if constexpr (N < 8) {
        // 5-7 bytes: overlapping 4-byte reads
        uint32_t lo, hi;
        __builtin_memcpy(&lo, p, 4);
        __builtin_memcpy(&hi, p + N - 4, 4);
        uint64_t a = (static_cast<uint64_t>(lo) << 32) | hi;
        return wymix(a ^ wyp0 ^ N, INIT ^ wyp1);
    }
    else { // N == 8
        uint64_t v;
        __builtin_memcpy(&v, p, 8);
        return wymix(v ^ wyp0 ^ N, INIT ^ wyp1);
    }
}

// Variant C: Even simpler - single multiply always
template<size_t N>
[[gnu::always_inline]] inline uint64_t variant_c(const void* data) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);
    constexpr uint64_t secret1 = 0xa0761d6478bd642full;
    constexpr uint64_t secret2 = 0xe7037ed1a0b428dbull;

    // Load all bytes into a single 64-bit value
    uint64_t v = 0;
    if constexpr (N >= 1) v |= static_cast<uint64_t>(p[0]);
    if constexpr (N >= 2) v |= static_cast<uint64_t>(p[1]) << 8;
    if constexpr (N >= 3) v |= static_cast<uint64_t>(p[2]) << 16;
    if constexpr (N >= 4) v |= static_cast<uint64_t>(p[3]) << 24;
    if constexpr (N >= 5) v |= static_cast<uint64_t>(p[4]) << 32;
    if constexpr (N >= 6) v |= static_cast<uint64_t>(p[5]) << 40;
    if constexpr (N >= 7) v |= static_cast<uint64_t>(p[6]) << 48;
    if constexpr (N >= 8) v |= static_cast<uint64_t>(p[7]) << 56;

    // Single multiply with length encoding
    __uint128_t r = static_cast<__uint128_t>(v ^ secret1 ^ N) * (secret2 ^ (N * secret1));
    return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
}

// Variant D: Optimized memcpy-based with proper finalization
template<size_t N>
[[gnu::always_inline]] inline uint64_t variant_d(const void* data) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);
    constexpr uint64_t wyp0 = 0xa0761d6478bd642full;
    constexpr uint64_t wyp1 = 0xe7037ed1a0b428dbull;

    if constexpr (N == 0) {
        return 0;
    }
    else if constexpr (N <= 8) {
        uint64_t a = 0;
        // Use memcpy for aligned access where possible
        if constexpr (N == 8) {
            __builtin_memcpy(&a, p, 8);
        } else if constexpr (N >= 4) {
            uint32_t lo;
            __builtin_memcpy(&lo, p, 4);
            uint32_t hi = 0;
            if constexpr (N > 4) {
                __builtin_memcpy(&hi, p + N - 4, 4);
            }
            a = (static_cast<uint64_t>(lo) << 32) | hi;
        } else {
            // 1-3 bytes: wyhash trick
            a = (static_cast<uint64_t>(p[0]) << 16) |
                (static_cast<uint64_t>(p[N >> 1]) << 8) |
                static_cast<uint64_t>(p[N - 1]);
        }
        // Direct single multiply (matches optimized 9-16 byte path style)
        __uint128_t r = static_cast<__uint128_t>(a ^ wyp0 ^ N) * (wyp1 ^ 0x1ff5c2923a788d2cull);
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    }
    else {
        return baseline<N>(data);
    }
}

} // namespace micro

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MICRO VARIANT COMPARISON (1-8 bytes)                                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝\n\n";

    constexpr size_t ITERS = 10000000;

    std::mt19937_64 rng(42);
    alignas(64) std::array<uint8_t, 8> data;
    for (auto& b : data) b = rng() & 0xFF;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(6) << "Size"
              << std::setw(12) << "Baseline"
              << std::setw(12) << "Variant B"
              << std::setw(12) << "Variant C"
              << std::setw(12) << "Variant D"
              << std::setw(10) << "Best"
              << "\n";
    std::cout << std::string(74, '-') << "\n";

    auto test_size = [&]<size_t N>() {
        alignas(64) std::array<uint8_t, N> d;
        for (size_t i = 0; i < N; ++i) d[i] = rng() & 0xFF;

        double t_base = bench([&]{ return micro::baseline<N>(d.data()); }, ITERS);
        double t_b = bench([&]{ return micro::variant_b<N>(d.data()); }, ITERS);
        double t_c = bench([&]{ return micro::variant_c<N>(d.data()); }, ITERS);
        double t_d = bench([&]{ return micro::variant_d<N>(d.data()); }, ITERS);

        double best = std::min({t_base, t_b, t_c, t_d});
        const char* best_name = (best == t_base) ? "Baseline" :
                                (best == t_b) ? "B" :
                                (best == t_c) ? "C" : "D";

        std::cout << std::setw(6) << N
                  << std::setw(11) << t_base << "ns"
                  << std::setw(11) << t_b << "ns"
                  << std::setw(11) << t_c << "ns"
                  << std::setw(11) << t_d << "ns"
                  << std::setw(10) << best_name
                  << "\n";
    };

    test_size.template operator()<1>();
    test_size.template operator()<2>();
    test_size.template operator()<3>();
    test_size.template operator()<4>();
    test_size.template operator()<5>();
    test_size.template operator()<6>();
    test_size.template operator()<7>();
    test_size.template operator()<8>();

    // Quality check
    std::cout << "\n═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "QUALITY CHECK (Avalanche, ideal = 0.5000)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << std::setprecision(4);
    std::cout << std::setw(6) << "Size"
              << std::setw(12) << "Baseline"
              << std::setw(12) << "Variant B"
              << std::setw(12) << "Variant C"
              << std::setw(12) << "Variant D"
              << "\n";
    std::cout << std::string(54, '-') << "\n";

    auto test_quality = [&]<size_t N>() {
        auto base_fn = [](const void* p, size_t) { return micro::baseline<N>(p); };
        auto b_fn = [](const void* p, size_t) { return micro::variant_b<N>(p); };
        auto c_fn = [](const void* p, size_t) { return micro::variant_c<N>(p); };
        auto d_fn = [](const void* p, size_t) { return micro::variant_d<N>(p); };

        double q_base = test_avalanche(base_fn, N, 5000);
        double q_b = test_avalanche(b_fn, N, 5000);
        double q_c = test_avalanche(c_fn, N, 5000);
        double q_d = test_avalanche(d_fn, N, 5000);

        auto status = [](double v) -> const char* {
            return (std::abs(v - 0.5) < 0.02) ? " ✓" : (std::abs(v - 0.5) < 0.05) ? " ~" : " ✗";
        };

        std::cout << std::setw(6) << N
                  << std::setw(10) << q_base << status(q_base)
                  << std::setw(10) << q_b << status(q_b)
                  << std::setw(10) << q_c << status(q_c)
                  << std::setw(10) << q_d << status(q_d)
                  << "\n";
    };

    test_quality.template operator()<1>();
    test_quality.template operator()<2>();
    test_quality.template operator()<3>();
    test_quality.template operator()<4>();
    test_quality.template operator()<5>();
    test_quality.template operator()<6>();
    test_quality.template operator()<7>();
    test_quality.template operator()<8>();

    std::cout << "\nLegend: ✓ = excellent (±0.02), ~ = acceptable (±0.05), ✗ = poor\n";

    return 0;
}
