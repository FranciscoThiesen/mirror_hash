// ============================================================================
// OPTIMIZATION TESTING BENCHMARK
// ============================================================================
// Tests each proposed optimization individually to measure impact
// ============================================================================

#include "mirror_hash/mirror_hash.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <cstring>
#include <array>
#include <unordered_set>

#if defined(__aarch64__)
#include <arm_neon.h>
#ifdef __ARM_FEATURE_CRYPTO
#include <arm_acle.h>
#define HAS_ARM_CRYPTO 1
#else
#define HAS_ARM_CRYPTO 0
#endif
#endif

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
// Quality test (avalanche)
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
// OPTIMIZATION 1: ARM Crypto Extension mixing (AES-like)
// ============================================================================
// Uses ARM's hardware AES for fast mixing on aarch64

namespace opt1_arm_crypto {

#if defined(__aarch64__) && HAS_ARM_CRYPTO
// ARM Crypto extension based mixing
inline uint64_t arm_crypto_mix(uint64_t a, uint64_t b) noexcept {
    // Pack into 128-bit vector
    uint8x16_t data = vreinterpretq_u8_u64(vcombine_u64(vcreate_u64(a), vcreate_u64(b)));
    // AES single round (provides excellent mixing)
    uint8x16_t key = vdupq_n_u8(0);
    data = vaeseq_u8(data, key);  // AES encrypt round
    data = vaesmcq_u8(data);       // AES mix columns
    // Extract result
    uint64x2_t result = vreinterpretq_u64_u8(data);
    return vgetq_lane_u64(result, 0) ^ vgetq_lane_u64(result, 1);
}

template<size_t N>
uint64_t hash_arm_crypto(const void* data) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);
    uint64_t seed = 0x1ff5c2923a788d2cull;

    if constexpr (N <= 16) {
        uint64_t a = 0, b = 0;
        if constexpr (N >= 8) {
            __builtin_memcpy(&a, p, 8);
            if constexpr (N > 8) __builtin_memcpy(&b, p + N - 8, 8);
        } else {
            __builtin_memcpy(&a, p, N);
        }
        return arm_crypto_mix(a ^ N, b ^ seed);
    } else {
        // Process 16 bytes at a time using AES
        size_t i = 0;
        for (; i + 16 <= N; i += 16) {
            uint64_t a, b;
            __builtin_memcpy(&a, p + i, 8);
            __builtin_memcpy(&b, p + i + 8, 8);
            seed = arm_crypto_mix(a ^ seed, b);
        }
        // Handle tail
        if (i < N) {
            uint64_t a, b;
            __builtin_memcpy(&a, p + N - 16, 8);
            __builtin_memcpy(&b, p + N - 8, 8);
            seed = arm_crypto_mix(a ^ seed, b ^ N);
        }
        return seed;
    }
}
#else
// Fallback - no ARM crypto
template<size_t N>
uint64_t hash_arm_crypto(const void* data) noexcept {
    return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, N>(data);
}
#endif

} // namespace opt1_arm_crypto

// ============================================================================
// OPTIMIZATION 2: NEON Vector Layer for large inputs
// ============================================================================

namespace opt2_neon_vector {

#if defined(__aarch64__) && defined(__ARM_NEON)
// NEON-accelerated hashing for large inputs using parallel 32x32->64 multiplies
template<size_t N>
uint64_t hash_neon_vector(const void* data) noexcept {
    if constexpr (N < 64) {
        // Fall back to scalar for small inputs
        return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, N>(data);
    } else {
        const auto* p = static_cast<const uint8_t*>(data);

        // Initialize 4 accumulators in NEON registers
        uint64x2_t acc0 = vdupq_n_u64(0xa0761d6478bd642full);
        uint64x2_t acc1 = vdupq_n_u64(0xe7037ed1a0b428dbull);

        // Secret keys for mixing
        uint32x4_t key = {0x78bd642f, 0xa0761d64, 0x0b428db, 0xe7037ed1};

        size_t i = 0;
        for (; i + 32 <= N; i += 32) {
            // Load 32 bytes (8 x 32-bit values)
            uint32x4_t d0 = vld1q_u32(reinterpret_cast<const uint32_t*>(p + i));
            uint32x4_t d1 = vld1q_u32(reinterpret_cast<const uint32_t*>(p + i + 16));

            // Parallel 32x32->64 multiplies (vmull_u32)
            uint64x2_t m0 = vmull_u32(vget_low_u32(d0), vget_low_u32(key));
            uint64x2_t m1 = vmull_u32(vget_high_u32(d0), vget_high_u32(key));
            uint64x2_t m2 = vmull_u32(vget_low_u32(d1), vget_low_u32(key));
            uint64x2_t m3 = vmull_u32(vget_high_u32(d1), vget_high_u32(key));

            // XOR into accumulators
            acc0 = veorq_u64(acc0, m0);
            acc0 = veorq_u64(acc0, m1);
            acc1 = veorq_u64(acc1, m2);
            acc1 = veorq_u64(acc1, m3);
        }

        // Combine accumulators
        uint64x2_t combined = veorq_u64(acc0, acc1);
        uint64_t seed = vgetq_lane_u64(combined, 0) ^ vgetq_lane_u64(combined, 1);

        // Handle remaining bytes with scalar code
        using P = mirror_hash::wyhash_policy;
        for (; i + 16 <= N; i += 16) {
            uint64_t a, b;
            __builtin_memcpy(&a, p + i, 8);
            __builtin_memcpy(&b, p + i + 8, 8);
            seed = P::wymix(a ^ P::wyp1, b ^ seed);
        }

        // Final bytes
        uint64_t a, b;
        __builtin_memcpy(&a, p + N - 16, 8);
        __builtin_memcpy(&b, p + N - 8, 8);
        return P::finalize_fast(seed, a, b, N);
    }
}
#else
template<size_t N>
uint64_t hash_neon_vector(const void* data) noexcept {
    return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, N>(data);
}
#endif

} // namespace opt2_neon_vector

// ============================================================================
// OPTIMIZATION 3: 128-bit State Accumulation
// ============================================================================
// Delay folding, carry two 64-bit halves through the main loop

namespace opt3_128bit_state {

template<size_t N>
uint64_t hash_128bit_state(const void* data) noexcept {
    if constexpr (N < 32) {
        return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, N>(data);
    } else {
        const auto* p = static_cast<const uint8_t*>(data);
        using P = mirror_hash::wyhash_policy;

        // Two 64-bit accumulators instead of one
        uint64_t lo_acc = P::INIT_SEED;
        uint64_t hi_acc = P::wyp0;

        size_t i = 0;
        for (; i + 16 <= N; i += 16) {
            uint64_t a, b;
            __builtin_memcpy(&a, p + i, 8);
            __builtin_memcpy(&b, p + i + 8, 8);

            // Full 128-bit multiply, don't fold immediately
            __uint128_t r = static_cast<__uint128_t>(a ^ lo_acc ^ P::wyp1) * (b ^ hi_acc);

            // Accumulate both halves (XOR to preserve more state)
            lo_acc ^= static_cast<uint64_t>(r);
            hi_acc ^= static_cast<uint64_t>(r >> 64);
        }

        // Final folding at the end
        __uint128_t final_r = static_cast<__uint128_t>(lo_acc ^ P::wyp0 ^ N) * (hi_acc ^ P::wyp1);
        return static_cast<uint64_t>(final_r) ^ static_cast<uint64_t>(final_r >> 64);
    }
}

} // namespace opt3_128bit_state

// ============================================================================
// OPTIMIZATION 4: Constant Embedding (verify current is optimal)
// ============================================================================
// This is more of an assembly check - constants should be immediates

namespace opt4_const_embed {

// Force all constants to be in registers/immediates
template<size_t N>
[[gnu::noinline]] uint64_t hash_const_embed(const void* data) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);

    // Use 32-bit parts where possible for better immediate encoding
    constexpr uint32_t wyp0_lo = 0x78bd642f;
    constexpr uint32_t wyp0_hi = 0xa0761d64;
    constexpr uint32_t wyp1_lo = 0xa0b428db;
    constexpr uint32_t wyp1_hi = 0xe7037ed1;
    constexpr uint64_t wyp0 = (static_cast<uint64_t>(wyp0_hi) << 32) | wyp0_lo;
    constexpr uint64_t wyp1 = (static_cast<uint64_t>(wyp1_hi) << 32) | wyp1_lo;
    constexpr uint64_t INIT_SEED = 0x1ff5c2923a788d2cull;

    auto wymix = [](uint64_t a, uint64_t b) noexcept -> uint64_t {
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    };

    if constexpr (N <= 16) {
        uint64_t a = 0, b = 0;
        if constexpr (N >= 8) {
            __builtin_memcpy(&a, p, 8);
            if constexpr (N > 8) __builtin_memcpy(&b, p + N - 8, 8);
        } else if constexpr (N >= 4) {
            uint32_t v;
            __builtin_memcpy(&v, p, 4);
            a = v;
        } else {
            for (size_t i = 0; i < N; ++i) a |= static_cast<uint64_t>(p[i]) << (i * 8);
        }
        return wymix(a ^ wyp0 ^ N, b ^ wyp1 ^ INIT_SEED);
    } else {
        uint64_t seed = INIT_SEED;
        size_t i = 0;
        for (; i + 16 <= N; i += 16) {
            uint64_t a, b;
            __builtin_memcpy(&a, p + i, 8);
            __builtin_memcpy(&b, p + i + 8, 8);
            seed = wymix(a ^ wyp1, b ^ seed);
        }
        uint64_t a, b;
        __builtin_memcpy(&a, p + N - 16, 8);
        __builtin_memcpy(&b, p + N - 8, 8);
        return wymix(a ^ wyp0 ^ N, b ^ wyp1 ^ seed);
    }
}

} // namespace opt4_const_embed

// ============================================================================
// OPTIMIZATION 5: Improved Prefetching
// ============================================================================
// Use NTA hints and better look-ahead distances

namespace opt5_prefetch {

template<size_t N>
uint64_t hash_improved_prefetch(const void* data) noexcept {
    if constexpr (N < 256) {
        return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, N>(data);
    } else {
        const auto* p = static_cast<const uint8_t*>(data);
        using P = mirror_hash::wyhash_policy;

        uint64_t seed = P::INIT_SEED;

        // Aggressive prefetching with NTA (non-temporal) for streaming data
        __builtin_prefetch(p + 256, 0, 0);  // NTA hint
        __builtin_prefetch(p + 320, 0, 0);
        if constexpr (N >= 512) {
            __builtin_prefetch(p + 384, 0, 0);
            __builtin_prefetch(p + 448, 0, 0);
        }

        size_t i = 0;
        for (; i + 64 <= N; i += 64) {
            // Prefetch 4 cache lines ahead
            if (i + 256 < N) {
                __builtin_prefetch(p + i + 256, 0, 0);
            }

            uint64_t w0, w1, w2, w3, w4, w5, w6, w7;
            __builtin_memcpy(&w0, p + i, 8);
            __builtin_memcpy(&w1, p + i + 8, 8);
            __builtin_memcpy(&w2, p + i + 16, 8);
            __builtin_memcpy(&w3, p + i + 24, 8);
            __builtin_memcpy(&w4, p + i + 32, 8);
            __builtin_memcpy(&w5, p + i + 40, 8);
            __builtin_memcpy(&w6, p + i + 48, 8);
            __builtin_memcpy(&w7, p + i + 56, 8);

            seed = P::wymix(w0 ^ P::wyp1, w1 ^ seed);
            seed = P::wymix(w2 ^ P::wyp2, w3 ^ seed);
            seed = P::wymix(w4 ^ P::wyp3, w5 ^ seed);
            seed = P::wymix(w6 ^ P::wyp0, w7 ^ seed);
        }

        // Handle remainder
        for (; i + 16 <= N; i += 16) {
            uint64_t a, b;
            __builtin_memcpy(&a, p + i, 8);
            __builtin_memcpy(&b, p + i + 8, 8);
            seed = P::wymix(a ^ P::wyp1, b ^ seed);
        }

        uint64_t a, b;
        __builtin_memcpy(&a, p + N - 16, 8);
        __builtin_memcpy(&b, p + N - 8, 8);
        return P::finalize_fast(seed, a, b, N);
    }
}

} // namespace opt5_prefetch

// ============================================================================
// OPTIMIZATION 6: Zero-Protection Enhancement
// ============================================================================
// Ensure all multiply operands have non-zero protection

namespace opt6_zero_protect {

template<size_t N>
uint64_t hash_zero_protect(const void* data) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);

    constexpr uint64_t wyp0 = 0xa0761d6478bd642full;
    constexpr uint64_t wyp1 = 0xe7037ed1a0b428dbull;
    constexpr uint64_t wyp2 = 0x8ebc6af09c88c6e3ull;  // Additional protection
    constexpr uint64_t INIT_SEED = 0x1ff5c2923a788d2cull;

    auto wymix_protected = [](uint64_t a, uint64_t b) noexcept -> uint64_t {
        // Both operands protected from zero
        a |= 1;  // Ensure non-zero (minimal overhead)
        b |= 1;
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    };

    auto wymix = [](uint64_t a, uint64_t b) noexcept -> uint64_t {
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    };

    if constexpr (N <= 16) {
        uint64_t a = 0, b = 0;
        if constexpr (N >= 8) {
            __builtin_memcpy(&a, p, 8);
            if constexpr (N > 8) __builtin_memcpy(&b, p + N - 8, 8);
        } else {
            for (size_t i = 0; i < N; ++i) a |= static_cast<uint64_t>(p[i]) << (i * 8);
        }
        // Double protection: XOR with different secrets ensures non-zero
        return wymix((a ^ wyp0 ^ N) | 1, (b ^ wyp1 ^ INIT_SEED) | 1);
    } else {
        uint64_t seed = INIT_SEED;
        size_t i = 0;
        for (; i + 16 <= N; i += 16) {
            uint64_t a, b;
            __builtin_memcpy(&a, p + i, 8);
            __builtin_memcpy(&b, p + i + 8, 8);
            // Use wyp2 as additional protection for second operand
            seed = wymix((a ^ wyp1) | 1, (b ^ seed ^ wyp2) | 1);
        }
        uint64_t a, b;
        __builtin_memcpy(&a, p + N - 16, 8);
        __builtin_memcpy(&b, p + N - 8, 8);
        return wymix((a ^ wyp0 ^ N) | 1, (b ^ wyp1 ^ seed) | 1);
    }
}

} // namespace opt6_zero_protect

// ============================================================================
// OPTIMIZATION 7: Branchless Tail Handling
// ============================================================================

namespace opt7_branchless {

template<size_t N>
uint64_t hash_branchless(const void* data) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);
    using P = mirror_hash::wyhash_policy;

    if constexpr (N == 0) {
        return 0;
    }
    // Branchless read for 1-8 bytes using mask
    else if constexpr (N <= 8) {
        uint64_t v = 0;
        // Load with potential overread, then mask
        if constexpr (N >= 4) {
            uint32_t lo;
            __builtin_memcpy(&lo, p, 4);
            v = lo;
            if constexpr (N > 4) {
                uint32_t hi;
                __builtin_memcpy(&hi, p + N - 4, 4);
                v |= static_cast<uint64_t>(hi) << 32;
            }
        } else {
            // For 1-3 bytes, use wyhash trick
            v = (static_cast<uint64_t>(p[0]) << 16) |
                (static_cast<uint64_t>(p[N >> 1]) << 8) |
                static_cast<uint64_t>(p[N - 1]);
        }
        return P::wymix(v ^ P::wyp0 ^ N, P::INIT_SEED ^ P::wyp1);
    }
    else if constexpr (N <= 16) {
        uint64_t a, b;
        __builtin_memcpy(&a, p, 8);
        __builtin_memcpy(&b, p + N - 8, 8);
        return P::wymix(a ^ P::wyp0 ^ N, b ^ P::INIT_SEED ^ P::wyp1);
    }
    else {
        // Fall back to regular for larger sizes
        return mirror_hash::detail::hash_bytes_fixed<P, N>(data);
    }
}

} // namespace opt7_branchless

// ============================================================================
// OPTIMIZATION 8: Micro Variant for Tiny Inputs
// ============================================================================
// Simplified path for 0-7 bytes with minimal setup

namespace opt8_micro {

template<size_t N>
[[gnu::always_inline]] inline uint64_t hash_micro(const void* data) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);

    // Constants as 32-bit where possible
    constexpr uint64_t secret1 = 0xa0761d6478bd642full;
    constexpr uint64_t secret2 = 0xe7037ed1a0b428dbull;

    if constexpr (N == 0) {
        return secret1;
    }
    else if constexpr (N == 1) {
        return p[0] * secret1;  // Single byte: just multiply
    }
    else if constexpr (N == 2) {
        uint16_t v;
        __builtin_memcpy(&v, p, 2);
        return v * secret1;
    }
    else if constexpr (N == 3) {
        // wyhash 3-byte trick
        uint64_t v = (static_cast<uint64_t>(p[0]) << 16) |
                     (static_cast<uint64_t>(p[1]) << 8) |
                     static_cast<uint64_t>(p[2]);
        __uint128_t r = static_cast<__uint128_t>(v ^ secret1) * secret2;
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    }
    else if constexpr (N == 4) {
        uint32_t v;
        __builtin_memcpy(&v, p, 4);
        __uint128_t r = static_cast<__uint128_t>(v ^ secret1) * secret2;
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    }
    else if constexpr (N <= 7) {
        // 5-7 bytes: overlapping 4-byte reads
        uint32_t lo, hi;
        __builtin_memcpy(&lo, p, 4);
        __builtin_memcpy(&hi, p + N - 4, 4);
        uint64_t v = (static_cast<uint64_t>(lo) << 32) | hi;
        __uint128_t r = static_cast<__uint128_t>(v ^ secret1 ^ N) * secret2;
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    }
    else if constexpr (N == 8) {
        uint64_t v;
        __builtin_memcpy(&v, p, 8);
        __uint128_t r = static_cast<__uint128_t>(v ^ secret1) * (secret2 ^ N);
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    }
    else {
        // Fall back for larger
        return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, N>(data);
    }
}

} // namespace opt8_micro

// ============================================================================
// Main benchmark
// ============================================================================

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  OPTIMIZATION TESTING BENCHMARK                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝\n\n";

#if defined(__aarch64__)
    std::cout << "Platform: aarch64 (ARM64)\n";
#if HAS_ARM_CRYPTO
    std::cout << "ARM Crypto Extensions: AVAILABLE\n";
#else
    std::cout << "ARM Crypto Extensions: NOT AVAILABLE\n";
#endif
#if defined(__ARM_NEON)
    std::cout << "ARM NEON: AVAILABLE\n";
#endif
#endif
    std::cout << "\n";

    // Prepare test data
    std::mt19937_64 rng(42);

    alignas(64) std::array<uint8_t, 8> data8;
    alignas(64) std::array<uint8_t, 16> data16;
    alignas(64) std::array<uint8_t, 32> data32;
    alignas(64) std::array<uint8_t, 64> data64;
    alignas(64) std::array<uint8_t, 128> data128;
    alignas(64) std::array<uint8_t, 256> data256;
    alignas(64) std::array<uint8_t, 512> data512;
    alignas(64) std::array<uint8_t, 1024> data1024;

    auto fill = [&](auto& arr) {
        for (auto& b : arr) b = rng() & 0xFF;
    };
    fill(data8); fill(data16); fill(data32); fill(data64);
    fill(data128); fill(data256); fill(data512); fill(data1024);

    constexpr size_t ITERS = 5000000;

    // Baseline function
    auto baseline_fn = [](const void* p, size_t len) -> uint64_t {
        return mirror_hash::detail::hash_bytes<mirror_hash::wyhash_policy>(p, len);
    };

    std::cout << std::fixed << std::setprecision(2);

    // ========================================================================
    // Test each optimization
    // ========================================================================

    auto test_optimization = [&]<size_t SIZE>(const char* name, auto opt_fn, std::array<uint8_t, SIZE>& data) {
        double t_baseline = bench([&]{
            return mirror_hash::detail::hash_bytes_fixed<mirror_hash::wyhash_policy, SIZE>(data.data());
        }, ITERS);

        double t_opt = bench([&]{ return opt_fn(data.data()); }, ITERS);

        // Quality check
        auto baseline_hash_fn = [](const void* p, size_t len) -> uint64_t {
            return mirror_hash::detail::hash_bytes<mirror_hash::wyhash_policy>(p, len);
        };
        double baseline_aval = test_avalanche(baseline_hash_fn, data.data(), SIZE);

        auto opt_hash_fn = [&](const void* p, size_t len) -> uint64_t {
            return opt_fn(p);
        };
        double opt_aval = test_avalanche(opt_hash_fn, data.data(), SIZE);

        double speedup = (t_baseline / t_opt - 1.0) * 100.0;
        bool quality_ok = std::abs(opt_aval - 0.5) < 0.05;

        std::cout << std::setw(30) << name
                  << std::setw(8) << SIZE << "B"
                  << std::setw(10) << t_baseline << "ns"
                  << std::setw(10) << t_opt << "ns"
                  << std::setw(10) << (speedup >= 0 ? "+" : "") << speedup << "%"
                  << std::setw(8) << opt_aval
                  << (quality_ok ? "  ✓" : "  ✗")
                  << "\n";
    };

    std::cout << std::setw(30) << "Optimization"
              << std::setw(9) << "Size"
              << std::setw(11) << "Baseline"
              << std::setw(11) << "Optimized"
              << std::setw(11) << "Speedup"
              << std::setw(9) << "Aval"
              << "  Quality\n";
    std::cout << std::string(90, '-') << "\n";

    // Optimization 1: ARM Crypto
    std::cout << "\n[OPT 1] ARM Crypto Extension Mixing:\n";
    test_optimization("ARM Crypto", opt1_arm_crypto::hash_arm_crypto<8>, data8);
    test_optimization("ARM Crypto", opt1_arm_crypto::hash_arm_crypto<16>, data16);
    test_optimization("ARM Crypto", opt1_arm_crypto::hash_arm_crypto<64>, data64);
    test_optimization("ARM Crypto", opt1_arm_crypto::hash_arm_crypto<256>, data256);

    // Optimization 2: NEON Vector
    std::cout << "\n[OPT 2] NEON Vector Layer:\n";
    test_optimization("NEON Vector", opt2_neon_vector::hash_neon_vector<64>, data64);
    test_optimization("NEON Vector", opt2_neon_vector::hash_neon_vector<256>, data256);
    test_optimization("NEON Vector", opt2_neon_vector::hash_neon_vector<512>, data512);
    test_optimization("NEON Vector", opt2_neon_vector::hash_neon_vector<1024>, data1024);

    // Optimization 3: 128-bit State
    std::cout << "\n[OPT 3] 128-bit State Accumulation:\n";
    test_optimization("128-bit State", opt3_128bit_state::hash_128bit_state<32>, data32);
    test_optimization("128-bit State", opt3_128bit_state::hash_128bit_state<64>, data64);
    test_optimization("128-bit State", opt3_128bit_state::hash_128bit_state<256>, data256);
    test_optimization("128-bit State", opt3_128bit_state::hash_128bit_state<512>, data512);

    // Optimization 4: Constant Embedding
    std::cout << "\n[OPT 4] Constant Embedding:\n";
    test_optimization("Const Embed", opt4_const_embed::hash_const_embed<8>, data8);
    test_optimization("Const Embed", opt4_const_embed::hash_const_embed<16>, data16);
    test_optimization("Const Embed", opt4_const_embed::hash_const_embed<64>, data64);
    test_optimization("Const Embed", opt4_const_embed::hash_const_embed<256>, data256);

    // Optimization 5: Improved Prefetching
    std::cout << "\n[OPT 5] Improved Prefetching:\n";
    test_optimization("Prefetch NTA", opt5_prefetch::hash_improved_prefetch<256>, data256);
    test_optimization("Prefetch NTA", opt5_prefetch::hash_improved_prefetch<512>, data512);
    test_optimization("Prefetch NTA", opt5_prefetch::hash_improved_prefetch<1024>, data1024);

    // Optimization 6: Zero Protection
    std::cout << "\n[OPT 6] Zero-Protection:\n";
    test_optimization("Zero Protect", opt6_zero_protect::hash_zero_protect<8>, data8);
    test_optimization("Zero Protect", opt6_zero_protect::hash_zero_protect<16>, data16);
    test_optimization("Zero Protect", opt6_zero_protect::hash_zero_protect<64>, data64);
    test_optimization("Zero Protect", opt6_zero_protect::hash_zero_protect<256>, data256);

    // Optimization 7: Branchless Tail
    std::cout << "\n[OPT 7] Branchless Tail Handling:\n";
    test_optimization("Branchless", opt7_branchless::hash_branchless<8>, data8);
    test_optimization("Branchless", opt7_branchless::hash_branchless<16>, data16);

    // Optimization 8: Micro Variant
    std::cout << "\n[OPT 8] Micro Variant (tiny inputs):\n";
    alignas(64) std::array<uint8_t, 1> data1 = {42};
    alignas(64) std::array<uint8_t, 2> data2 = {42, 43};
    alignas(64) std::array<uint8_t, 3> data3 = {42, 43, 44};
    alignas(64) std::array<uint8_t, 4> data4 = {42, 43, 44, 45};
    alignas(64) std::array<uint8_t, 5> data5 = {42, 43, 44, 45, 46};
    alignas(64) std::array<uint8_t, 6> data6 = {42, 43, 44, 45, 46, 47};
    alignas(64) std::array<uint8_t, 7> data7 = {42, 43, 44, 45, 46, 47, 48};

    test_optimization("Micro", opt8_micro::hash_micro<1>, data1);
    test_optimization("Micro", opt8_micro::hash_micro<2>, data2);
    test_optimization("Micro", opt8_micro::hash_micro<3>, data3);
    test_optimization("Micro", opt8_micro::hash_micro<4>, data4);
    test_optimization("Micro", opt8_micro::hash_micro<5>, data5);
    test_optimization("Micro", opt8_micro::hash_micro<6>, data6);
    test_optimization("Micro", opt8_micro::hash_micro<7>, data7);
    test_optimization("Micro", opt8_micro::hash_micro<8>, data8);

    std::cout << "\n═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "Legend: Speedup > 0% = faster than baseline, Aval ≈ 0.50 = good quality\n";
    std::cout << "        ✓ = quality acceptable (|aval - 0.5| < 0.05)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";

    return 0;
}
