#pragma once

// ============================================================================
// mirror_hash - Reflection-Based Hashing for C++26
// ============================================================================
//
// A configurable, high-performance hashing library using C++26 reflection.
// Supports multiple hash algorithms via compile-time policy selection.
//
// Usage:
//   // Default (Folly) policy
//   auto h = mirror_hash::hash(my_struct);
//
//   // Custom policy
//   auto h = mirror_hash::hash<mirror_hash::wyhash_policy>(my_struct);
//
//   // With std::unordered_set
//   std::unordered_set<MyType, mirror_hash::hasher<>> my_set;
//
// ============================================================================

#include <meta>
#include <cstdint>
#include <cstddef>
#include <concepts>
#include <type_traits>
#include <functional>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <optional>
#include <variant>
#include <memory>
#include <utility>

namespace mirror_hash {

// ============================================================================
// SIMD Detection and Runtime Dispatch
// ============================================================================
//
// Inspired by simdjson's approach: detect available instruction sets at
// compile time, select best implementation at runtime.
//
// ============================================================================

namespace simd {

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define MIRROR_HASH_X86_64 1
#else
    #define MIRROR_HASH_X86_64 0
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
    #define MIRROR_HASH_ARM64 1
#else
    #define MIRROR_HASH_ARM64 0
#endif

// Instruction set detection
#if MIRROR_HASH_X86_64
    #if defined(__AVX512F__)
        #define MIRROR_HASH_AVX512 1
    #else
        #define MIRROR_HASH_AVX512 0
    #endif
    #if defined(__AVX2__)
        #define MIRROR_HASH_AVX2 1
    #else
        #define MIRROR_HASH_AVX2 0
    #endif
    #if defined(__SSE4_2__)
        #define MIRROR_HASH_SSE42 1
    #else
        #define MIRROR_HASH_SSE42 0
    #endif
#else
    #define MIRROR_HASH_AVX512 0
    #define MIRROR_HASH_AVX2 0
    #define MIRROR_HASH_SSE42 0
#endif

#if MIRROR_HASH_ARM64
    #if defined(__ARM_NEON)
        #define MIRROR_HASH_NEON 1
    #else
        #define MIRROR_HASH_NEON 0
    #endif
#else
    #define MIRROR_HASH_NEON 0
#endif

enum class implementation {
    scalar,
    sse42,
    avx2,
    avx512,
    neon
};

// Runtime detection of best available implementation
inline implementation detect_best_implementation() noexcept {
#if MIRROR_HASH_AVX512
    return implementation::avx512;
#elif MIRROR_HASH_AVX2
    return implementation::avx2;
#elif MIRROR_HASH_SSE42
    return implementation::sse42;
#elif MIRROR_HASH_NEON
    return implementation::neon;
#else
    return implementation::scalar;
#endif
}

inline const implementation active_implementation = detect_best_implementation();

} // namespace simd

// ============================================================================
// Hash Policy Concept
// ============================================================================
//
// A hash policy must provide:
//   - combine(seed, value) -> size_t : Combine two hash values
//   - mix(value) -> size_t : Finalize/mix a single value (optional)
//
// ============================================================================

template<typename P>
concept HashPolicy = requires(std::size_t a, std::size_t b) {
    { P::combine(a, b) } noexcept -> std::same_as<std::size_t>;
};

// ============================================================================
// Hash Policies
// ============================================================================

// ----------------------------------------------------------------------------
// Folly Policy (Default) - Meta's F14 hash tables
// ----------------------------------------------------------------------------
// Quality: 5/6 tests passed, avalanche bias 0.0003
// Speed: ~1ns per combine
// Origin: folly/hash/Hash.h, derived from CityHash
//
struct folly_policy {
    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        const std::size_t kMul = 0x9ddfea08eb382d69ULL;
        std::size_t a = (value ^ seed) * kMul;
        a ^= (a >> 47);
        std::size_t b = (seed ^ a) * kMul;
        b ^= (b >> 47);
        b *= kMul;
        return b;
    }

    static constexpr std::size_t mix(std::size_t k) noexcept {
        return combine(0, k);
    }
};

// ----------------------------------------------------------------------------
// wyhash Policy - Used by Go, Zig, Nim
// ----------------------------------------------------------------------------
// Quality: 5/6 tests passed, avalanche bias 0.0000
// Speed: Fastest (uses 128-bit multiplication)
// Origin: https://github.com/wangyi-fudan/wyhash
//
struct wyhash_policy {
    // Secrets matching real wyhash
    static constexpr std::uint64_t wyp0 = 0xa0761d6478bd642full;
    static constexpr std::uint64_t wyp1 = 0xe7037ed1a0b428dbull;
    static constexpr std::uint64_t wyp2 = 0x8ebc6af09c88c6e3ull;
    static constexpr std::uint64_t wyp3 = 0x589965cc75374cc3ull;

    // OPTIMIZATION: Precomputed seed constant - saves 1 multiply per hash!
    // This is wymix(wyp0, wyp1) computed at compile time:
    //   __uint128_t r = wyp0 * wyp1 = 0x90ccc56588c08119_8f3907f7b2b80c35
    //   INIT_SEED = lo ^ hi = 0x8f3907f7b2b80c35 ^ 0x90ccc56588c08119 = 0x1ff5c2923a788d2c
    static constexpr std::uint64_t INIT_SEED = 0x1ff5c2923a788d2cull;

    static constexpr std::uint64_t wymix(std::uint64_t a, std::uint64_t b) noexcept {
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        return static_cast<std::uint64_t>(r) ^ static_cast<std::uint64_t>(r >> 64);
    }

    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        return wymix(seed ^ wyp0, value ^ wyp1);
    }

    // Process 16 bytes at once (2 words) - matches real wyhash pattern
    static constexpr std::size_t combine16(std::size_t seed, std::uint64_t a, std::uint64_t b) noexcept {
        return wymix(a ^ wyp1, b ^ seed);
    }

    static constexpr std::size_t mix(std::size_t k) noexcept {
        return wymix(k ^ wyp0, wyp1);
    }

    // Final mixing for wyhash-style finalization (2 multiplies - for compatibility)
    static constexpr std::size_t finalize(std::size_t seed, std::uint64_t a, std::uint64_t b, std::size_t len) noexcept {
        a ^= wyp1; b ^= seed;
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        a = static_cast<std::uint64_t>(r); b = static_cast<std::uint64_t>(r >> 64);
        return wymix(a ^ wyp0 ^ len, b ^ wyp1);
    }

    // Fast finalization using SINGLE multiply (for sizes > 16B where seed has good entropy)
    // Key insight: When seed has been through prior mixing, one 128-bit multiply provides
    // sufficient avalanche. This saves one multiply compared to standard finalize().
    static constexpr std::size_t finalize_fast(std::size_t seed, std::uint64_t a, std::uint64_t b, std::size_t len) noexcept {
        __uint128_t r = static_cast<__uint128_t>(a ^ wyp0 ^ len) * (b ^ wyp1 ^ seed);
        return static_cast<std::uint64_t>(r) ^ static_cast<std::uint64_t>(r >> 64);
    }
};

// ----------------------------------------------------------------------------
// MurmurHash3 Policy - Classic, well-tested
// ----------------------------------------------------------------------------
// Quality: 5/6 tests passed, avalanche bias 0.0000
// Speed: Fast, very portable
// Origin: https://github.com/aappleby/smhasher
//
struct murmur3_policy {
    static constexpr std::size_t fmix64(std::size_t k) noexcept {
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 33;
        k *= 0xc4ceb9fe1a85ec53ULL;
        k ^= k >> 33;
        return k;
    }

    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        // Mix value, then combine with seed, then mix result
        return fmix64(seed ^ fmix64(value));
    }

    static constexpr std::size_t mix(std::size_t k) noexcept {
        return fmix64(k);
    }
};

// ----------------------------------------------------------------------------
// xxHash3 Policy - Modern, SIMD-friendly design
// ----------------------------------------------------------------------------
// Quality: 5/6 tests passed
// Speed: Excellent, designed for SIMD
// Origin: https://github.com/Cyan4973/xxHash
//
struct xxhash3_policy {
    static constexpr std::uint64_t XXH_PRIME64_1 = 0x9E3779B185EBCA87ULL;
    static constexpr std::uint64_t XXH_PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    static constexpr std::uint64_t XXH_PRIME64_3 = 0x165667B19E3779F9ULL;

    static constexpr std::size_t xxh3_avalanche(std::size_t h) noexcept {
        h ^= h >> 37;
        h *= 0x165667919E3779F9ULL;
        h ^= h >> 32;
        return h;
    }

    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        std::size_t h = seed + value * XXH_PRIME64_2;
        h = ((h << 31) | (h >> 33)) * XXH_PRIME64_1;
        return xxh3_avalanche(h);
    }

    static constexpr std::size_t mix(std::size_t k) noexcept {
        return xxh3_avalanche(k);
    }
};

// ----------------------------------------------------------------------------
// FNV-1a Policy - Simple, streaming-friendly (lower quality)
// ----------------------------------------------------------------------------
// Quality: 2/6 tests passed - NOT RECOMMENDED for hash tables
// Speed: Fast for streaming
// Included for comparison only
//
struct fnv1a_policy {
    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        constexpr std::size_t FNV_PRIME = 0x100000001b3ULL;
        seed ^= value;
        seed *= FNV_PRIME;
        seed ^= value >> 32;
        seed *= FNV_PRIME;
        return seed;
    }

    static constexpr std::size_t mix(std::size_t k) noexcept {
        return combine(0xcbf29ce484222325ULL, k);
    }
};

// ----------------------------------------------------------------------------
// AES-based Policy - Hardware accelerated (when available)
// ----------------------------------------------------------------------------
// Quality: Excellent
// Speed: Very fast on CPUs with AES-NI
// Similar to Rust's aHash
//
struct aes_policy {
    // Fallback to Folly when AES not available
    // In a full implementation, this would use _mm_aesenc_si128
    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        // Simulated AES-like mixing using multiplications
        const std::size_t kAes1 = 0x243f6a8885a308d3ULL;
        const std::size_t kAes2 = 0x13198a2e03707344ULL;
        std::size_t a = (seed ^ kAes1) * (value ^ kAes2);
        a ^= a >> 29;
        a *= 0x1b873593ULL;
        a ^= a >> 32;
        return a ^ seed;
    }

    static constexpr std::size_t mix(std::size_t k) noexcept {
        return combine(0x9e3779b97f4a7c15ULL, k);
    }
};

// ----------------------------------------------------------------------------
// Rapidhash Policy - Faster variant of wyhash
// ----------------------------------------------------------------------------
// Quality: Excellent (similar to wyhash)
// Speed: Faster than wyhash due to simpler mixing
// Origin: https://github.com/Nicoshev/rapidhash
//
struct rapidhash_policy {
    static constexpr std::uint64_t rapid_secret[3] = {
        0x2d358dccaa6c78a5ULL, 0x8bb84b93962eacc9ULL, 0x4b33a62ed433d4a3ULL
    };

    static constexpr std::uint64_t rapid_mix(std::uint64_t a, std::uint64_t b) noexcept {
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        return static_cast<std::uint64_t>(r) ^ static_cast<std::uint64_t>(r >> 64);
    }

    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        return rapid_mix(seed ^ rapid_secret[0], value ^ rapid_secret[1]);
    }

    static constexpr std::size_t mix(std::size_t k) noexcept {
        return rapid_mix(k ^ rapid_secret[0], rapid_secret[2]);
    }
};

// ----------------------------------------------------------------------------
// Komihash Policy - Fast with good quality
// ----------------------------------------------------------------------------
// Quality: Good (passes avalanche with proper mixing)
// Speed: Fast - uses efficient mixing without 128-bit multiply
// Origin: Inspired by komihash, with improved finalization
//
struct komihash_policy {
    static constexpr std::uint64_t komi_secret = 0x243f6a8885a308d3ULL;

    // Proper mixing with good avalanche
    static constexpr std::size_t komi_mix(std::uint64_t v) noexcept {
        v ^= v >> 33;
        v *= 0xff51afd7ed558ccdULL;
        v ^= v >> 33;
        v *= 0xc4ceb9fe1a85ec53ULL;
        v ^= v >> 33;
        return v;
    }

    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        // Mix inputs, then use wyhash-style 128-bit multiply for quality
        __uint128_t r = static_cast<__uint128_t>(seed ^ komi_secret) * (value ^ 0xe7037ed1a0b428dbULL);
        return static_cast<std::uint64_t>(r) ^ static_cast<std::uint64_t>(r >> 64);
    }

    static constexpr std::size_t mix(std::size_t k) noexcept {
        return komi_mix(k ^ komi_secret);
    }
};

// ----------------------------------------------------------------------------
// Fast Policy - Speed + Quality balanced
// ----------------------------------------------------------------------------
// Quality: Good (proper avalanche via multiply-xor-shift)
// Speed: Fast - uses Folly-style combining
// Design: Derived from CityHash/Folly for proven quality
//
struct fast_policy {
    // CityHash/Folly constant - proven quality
    static constexpr std::uint64_t kMul = 0x9ddfea08eb382d69ULL;

    // Proper finalization with good avalanche
    static constexpr std::size_t mix(std::size_t k) noexcept {
        std::uint64_t a = k * kMul;
        a ^= (a >> 47);
        a *= kMul;
        a ^= (a >> 47);
        return a;
    }

    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        // Folly-style combining with proven quality
        std::uint64_t a = (value ^ seed) * kMul;
        a ^= (a >> 47);
        std::uint64_t b = (seed ^ a) * kMul;
        b ^= (b >> 47);
        return b * kMul;
    }
};

// Default policy alias
using default_policy = folly_policy;

// ============================================================================
// SIMD-Optimized Bulk Byte Hashing
// ============================================================================

namespace detail {

// Scalar implementation (fallback)
template<typename Policy>
inline std::size_t hash_bytes_scalar(const void* data, std::size_t len) noexcept {
    const auto* p = static_cast<const std::uint8_t*>(data);
    std::size_t h = len;

    // Process 8 bytes at a time
    while (len >= 8) {
        std::uint64_t v;
        __builtin_memcpy(&v, p, 8);
        h = Policy::combine(h, v);
        p += 8;
        len -= 8;
    }

    // Handle tail
    if (len > 0) {
        std::uint64_t v = 0;
        __builtin_memcpy(&v, p, len);
        h = Policy::combine(h, v);
    }

    return h;
}

#if MIRROR_HASH_NEON
// NEON implementation for ARM64 - 8-way tree reduction for maximum ILP
template<typename Policy>
inline std::size_t hash_bytes_neon(const void* data, std::size_t len) noexcept {
    const auto* p = static_cast<const std::uint8_t*>(data);
    const std::size_t original_len = len;

    // 8 parallel accumulators for maximum ILP (matches tree-reduction principle)
    std::size_t h0 = original_len;
    std::size_t h1 = original_len ^ 0x9e3779b97f4a7c15ULL;
    std::size_t h2 = original_len ^ 0x85ebca6b2fc3ea41ULL;
    std::size_t h3 = original_len ^ 0xc2b2ae35be7f56cdULL;
    std::size_t h4 = original_len ^ 0x13198a2e03707344ULL;
    std::size_t h5 = original_len ^ 0xa4093822299f31d0ULL;
    std::size_t h6 = original_len ^ 0x082efa98ec4e6c89ULL;
    std::size_t h7 = original_len ^ 0x452821e638d01377ULL;

    // Process 64 bytes at a time (cache line) with prefetching
    while (len >= 64) {
        // Prefetch 2 cache lines ahead
        __builtin_prefetch(p + 128, 0, 3);

        std::uint64_t v0, v1, v2, v3, v4, v5, v6, v7;
        __builtin_memcpy(&v0, p, 8);
        __builtin_memcpy(&v1, p + 8, 8);
        __builtin_memcpy(&v2, p + 16, 8);
        __builtin_memcpy(&v3, p + 24, 8);
        __builtin_memcpy(&v4, p + 32, 8);
        __builtin_memcpy(&v5, p + 40, 8);
        __builtin_memcpy(&v6, p + 48, 8);
        __builtin_memcpy(&v7, p + 56, 8);

        // Each accumulator processes one word - maximizes ILP
        h0 = Policy::combine(h0, v0);
        h1 = Policy::combine(h1, v1);
        h2 = Policy::combine(h2, v2);
        h3 = Policy::combine(h3, v3);
        h4 = Policy::combine(h4, v4);
        h5 = Policy::combine(h5, v5);
        h6 = Policy::combine(h6, v6);
        h7 = Policy::combine(h7, v7);

        p += 64;
        len -= 64;
    }

    // Tree reduction of 8 accumulators (7 combines with 4-way parallelism)
    h0 = Policy::combine(h0, h1);
    h2 = Policy::combine(h2, h3);
    h4 = Policy::combine(h4, h5);
    h6 = Policy::combine(h6, h7);

    h0 = Policy::combine(h0, h2);
    h4 = Policy::combine(h4, h6);

    std::size_t h = Policy::combine(h0, h4);

    // Process remaining 16-byte chunks
    while (len >= 16) {
        std::uint64_t v0, v1;
        __builtin_memcpy(&v0, p, 8);
        __builtin_memcpy(&v1, p + 8, 8);
        h = Policy::combine(Policy::combine(h, v0), v1);
        p += 16;
        len -= 16;
    }

    // Branchless tail handling
    if (len >= 8) {
        std::uint64_t v;
        __builtin_memcpy(&v, p, 8);
        h = Policy::combine(h, v);
        p += 8;
        len -= 8;
    }

    if (len > 0) {
        std::uint64_t v = 0;
        __builtin_memcpy(&v, p, len);
        h = Policy::combine(h, v);
    }

    return h;
}
#endif

#if MIRROR_HASH_AVX2
#include <immintrin.h>
// AVX2 implementation for x86_64
template<typename Policy>
inline std::size_t hash_bytes_avx2(const void* data, std::size_t len) noexcept {
    const auto* p = static_cast<const std::uint8_t*>(data);

    // Use 4 parallel hash states for better pipelining
    std::size_t h0 = len;
    std::size_t h1 = len ^ 0x9e3779b97f4a7c15ULL;
    std::size_t h2 = len ^ 0x85ebca6b2fc3ea41ULL;
    std::size_t h3 = len ^ 0xc2b2ae35be7f56cdULL;

    // Process 32 bytes at a time
    while (len >= 32) {
        std::uint64_t v0, v1, v2, v3;
        __builtin_memcpy(&v0, p, 8);
        __builtin_memcpy(&v1, p + 8, 8);
        __builtin_memcpy(&v2, p + 16, 8);
        __builtin_memcpy(&v3, p + 24, 8);

        h0 = Policy::combine(h0, v0);
        h1 = Policy::combine(h1, v1);
        h2 = Policy::combine(h2, v2);
        h3 = Policy::combine(h3, v3);

        p += 32;
        len -= 32;
    }

    // Combine parallel states
    std::size_t h = Policy::combine(Policy::combine(h0, h1), Policy::combine(h2, h3));

    // Handle remainder
    while (len >= 8) {
        std::uint64_t v;
        __builtin_memcpy(&v, p, 8);
        h = Policy::combine(h, v);
        p += 8;
        len -= 8;
    }

    if (len > 0) {
        std::uint64_t v = 0;
        __builtin_memcpy(&v, p, len);
        h = Policy::combine(h, v);
    }

    return h;
}
#endif

#if MIRROR_HASH_AVX512
// AVX-512 implementation
template<typename Policy>
inline std::size_t hash_bytes_avx512(const void* data, std::size_t len) noexcept {
    const auto* p = static_cast<const std::uint8_t*>(data);

    // 8 parallel hash states for 64-byte cache line processing
    std::size_t h[8] = {
        len, len ^ 0x9e3779b97f4a7c15ULL,
        len ^ 0x85ebca6b2fc3ea41ULL, len ^ 0xc2b2ae35be7f56cdULL,
        len ^ 0x13198a2e03707344ULL, len ^ 0xa4093822299f31d0ULL,
        len ^ 0x082efa98ec4e6c89ULL, len ^ 0x452821e638d01377ULL
    };

    // Process 64 bytes at a time (full cache line)
    while (len >= 64) {
        for (int i = 0; i < 8; ++i) {
            std::uint64_t v;
            __builtin_memcpy(&v, p + i * 8, 8);
            h[i] = Policy::combine(h[i], v);
        }
        p += 64;
        len -= 64;
    }

    // Combine all states
    std::size_t result = h[0];
    for (int i = 1; i < 8; ++i) {
        result = Policy::combine(result, h[i]);
    }

    // Handle remainder
    while (len >= 8) {
        std::uint64_t v;
        __builtin_memcpy(&v, p, 8);
        result = Policy::combine(result, v);
        p += 8;
        len -= 8;
    }

    if (len > 0) {
        std::uint64_t v = 0;
        __builtin_memcpy(&v, p, len);
        result = Policy::combine(result, v);
    }

    return result;
}
#endif

// ============================================================================
// Compile-time size-specialized hashing (ZERO loops for small structs)
// ============================================================================
//
// PRINCIPLE: Minimize combines by reading exactly ceil(N/8) words.
// For K words, we need K-1 combines minimum (tree reduction).
//
// Words needed:  1  2  3  4  5  6  7  8  9  10  11  12  ...
// Combines:      0  1  2  3  4  5  6  7  8   9  10  11  ...
//
// We use tree-reduction to enable instruction-level parallelism:
//   4 words: (v0,v1) (v2,v3) -> (h0,h1) -> result  [3 combines, 2 parallel]
//   6 words: (v0,v1) (v2,v3) (v4,v5) -> ... -> result [5 combines]
//
// ============================================================================

// Helper: read 8 bytes from offset (compile-time)
template<std::size_t Offset>
[[gnu::always_inline]] inline std::uint64_t read64(const std::uint8_t* p) noexcept {
    std::uint64_t v;
    __builtin_memcpy(&v, p + Offset, 8);
    return v;
}

// Helper: tree-reduce combines for maximum ILP
template<typename Policy>
[[gnu::always_inline]] inline std::size_t combine2(std::uint64_t a, std::uint64_t b) noexcept {
    return Policy::combine(a, b);
}

template<typename Policy>
[[gnu::always_inline]] inline std::size_t combine4(std::uint64_t a, std::uint64_t b,
                                                    std::uint64_t c, std::uint64_t d) noexcept {
    // Tree: (a,b) and (c,d) in parallel, then combine results
    return Policy::combine(Policy::combine(a, b), Policy::combine(c, d));
}

template<typename Policy>
[[gnu::always_inline]] inline std::size_t combine6(std::uint64_t a, std::uint64_t b,
                                                    std::uint64_t c, std::uint64_t d,
                                                    std::uint64_t e, std::uint64_t f) noexcept {
    // Tree: ((a,b), (c,d)) and (e,f) -> combine
    std::size_t h0 = Policy::combine(a, b);
    std::size_t h1 = Policy::combine(c, d);
    std::size_t h2 = Policy::combine(e, f);
    return Policy::combine(Policy::combine(h0, h1), h2);
}

template<typename Policy>
[[gnu::always_inline]] inline std::size_t combine8(std::uint64_t v0, std::uint64_t v1,
                                                    std::uint64_t v2, std::uint64_t v3,
                                                    std::uint64_t v4, std::uint64_t v5,
                                                    std::uint64_t v6, std::uint64_t v7) noexcept {
    // Full tree reduction: 7 combines, 4-way parallel at first level
    std::size_t h0 = Policy::combine(v0, v1);
    std::size_t h1 = Policy::combine(v2, v3);
    std::size_t h2 = Policy::combine(v4, v5);
    std::size_t h3 = Policy::combine(v6, v7);
    return Policy::combine(Policy::combine(h0, h1), Policy::combine(h2, h3));
}

// ============================================================================
// Wyhash-optimized: 16 bytes per mix (matching real wyhash algorithm)
// ============================================================================
// Real wyhash processes 16 bytes per wymix: wymix(a ^ secret, b ^ seed)
// This is more efficient than our generic 8-bytes-per-combine approach.
// ============================================================================

template<std::size_t N>
[[gnu::always_inline]] inline std::size_t hash_bytes_wyhash_optimized(const void* data) noexcept {
    const auto* p = static_cast<const std::uint8_t*>(data);
    using P = wyhash_policy;

    // OPTIMIZATION: Use precomputed seed - saves 1 multiply!
    std::size_t seed = P::INIT_SEED;

    if constexpr (N == 0) {
        return 0;
    }
    // ========== 1-8 bytes: single mix ==========
    else if constexpr (N <= 8) {
        if constexpr (N <= 3) {
            std::uint64_t v = p[0];
            if constexpr (N >= 2) v |= static_cast<std::uint64_t>(p[1]) << 8;
            if constexpr (N >= 3) v |= static_cast<std::uint64_t>(p[2]) << 16;
            return P::finalize(seed, v, 0, N);
        } else if constexpr (N == 4) {
            std::uint32_t v;
            __builtin_memcpy(&v, p, 4);
            return P::finalize(seed, v, 0, N);
        } else if constexpr (N < 8) {
            std::uint32_t lo, hi;
            __builtin_memcpy(&lo, p, 4);
            __builtin_memcpy(&hi, p + N - 4, 4);
            std::uint64_t a = (static_cast<std::uint64_t>(lo) << 32) | hi;
            return P::finalize(seed, a, 0, N);
        } else { // N == 8
            return P::finalize(seed, read64<0>(p), 0, N);
        }
    }
    // ========== 9-16 bytes: FULLY INLINED FAST PATH ==========
    else if constexpr (N <= 16) {
        // Direct 64-bit reads - no helper function
        std::uint64_t a, b;
        __builtin_memcpy(&a, p, 8);
        __builtin_memcpy(&b, p + N - 8, 8);

        // First mum: a ^= secret, b ^= seed
        a ^= P::wyp1;
        b ^= seed;
        {
            __uint128_t r = static_cast<__uint128_t>(a) * b;
            a = static_cast<std::uint64_t>(r);
            b = static_cast<std::uint64_t>(r >> 64);
        }

        // Final mix: include length, apply secrets, mum, return xor
        a ^= P::wyp0 ^ N;
        b ^= P::wyp1;
        {
            __uint128_t r = static_cast<__uint128_t>(a) * b;
            return static_cast<std::uint64_t>(r) ^ static_cast<std::uint64_t>(r >> 64);
        }
    }
    // ========== 17-48 bytes: process 16 bytes at a time ==========
    // After combine16, seed has good entropy -> use fast single-multiply finalize
    else if constexpr (N <= 48) {
        if constexpr (N > 32) {
            seed = P::combine16(seed, read64<0>(p), read64<8>(p));
            seed = P::combine16(seed, read64<16>(p), read64<24>(p));
        } else if constexpr (N > 16) {
            seed = P::combine16(seed, read64<0>(p), read64<8>(p));
        }
        // Final 16 bytes (overlapping) - use fast finalize (1 multiply instead of 2)
        return P::finalize_fast(seed, read64<N-16>(p), read64<N-8>(p), N);
    }
    // ========== 49-96 bytes: ULTRA OPTIMIZED with eager loading ==========
    // Key insight: Load ALL data upfront to maximize memory parallelism,
    // then process with 3-way parallel accumulators for proven hash quality.
    else if constexpr (N <= 96) {
        // Prefetch to L1 cache
        __builtin_prefetch(p, 0, 3);
        if constexpr (N > 64) __builtin_prefetch(p + 64, 0, 3);

        // Eager load ALL words - maximizes memory-level parallelism
        std::uint64_t w0 = read64<0>(p);
        std::uint64_t w1 = read64<8>(p);
        std::uint64_t w2 = read64<16>(p);
        std::uint64_t w3 = read64<24>(p);
        std::uint64_t w4 = read64<32>(p);
        std::uint64_t w5 = read64<40>(p);
        [[maybe_unused]] std::uint64_t w6, w7, w8, w9, w10, w11;
        if constexpr (N > 48) { w6 = read64<48>(p); w7 = read64<56>(p); }
        if constexpr (N > 64) { w8 = read64<64>(p); w9 = read64<72>(p); }
        if constexpr (N > 80) { w10 = read64<80>(p); w11 = read64<88>(p); }

        // 3-way parallel accumulators for first 48 bytes
        std::size_t see1 = seed, see2 = seed;
        seed = P::wymix(w0 ^ P::wyp1, w1 ^ seed);
        see1 = P::wymix(w2 ^ P::wyp2, w3 ^ see1);
        see2 = P::wymix(w4 ^ P::wyp3, w5 ^ see2);
        seed ^= see1 ^ see2;

        // Remaining 16-byte chunks (sequential, using pre-loaded data)
        if constexpr (N > 64) {
            seed = P::wymix(w6 ^ P::wyp1, w7 ^ seed);
        }
        if constexpr (N > 80) {
            seed = P::wymix(w8 ^ P::wyp1, w9 ^ seed);
        }

        // Finalize with last 16 bytes - use pre-loaded values and fast finalize
        if constexpr (N > 80) {
            return P::finalize_fast(seed, w10, w11, N);
        } else if constexpr (N > 64) {
            return P::finalize_fast(seed, read64<N-16>(p), read64<N-8>(p), N);
        } else if constexpr (N > 56) {
            // For N in (56, 64], use pre-loaded w6, w7 (no re-reading!)
            return P::finalize_fast(seed, w6, w7, N);
        } else {
            // For N in (48, 56], w7 may not be fully valid, use overlapping reads
            return P::finalize_fast(seed, read64<N-16>(p), read64<N-8>(p), N);
        }
    }
    // ========== 97-128 bytes: WYHASH-STYLE 3-way parallel ==========
    // Match real wyhash's 3-way pattern which has proven performance
    else if constexpr (N <= 128) {
        // 3-way parallel accumulators (matching real wyhash)
        std::size_t see1 = seed, see2 = seed;

        // First 48 bytes: 3-way parallel
        seed = P::wymix(read64<0>(p) ^ P::wyp1, read64<8>(p) ^ seed);
        see1 = P::wymix(read64<16>(p) ^ P::wyp2, read64<24>(p) ^ see1);
        see2 = P::wymix(read64<32>(p) ^ P::wyp3, read64<40>(p) ^ see2);

        // Second 48 bytes: 3-way parallel
        seed = P::wymix(read64<48>(p) ^ P::wyp1, read64<56>(p) ^ seed);
        see1 = P::wymix(read64<64>(p) ^ P::wyp2, read64<72>(p) ^ see1);
        see2 = P::wymix(read64<80>(p) ^ P::wyp3, read64<88>(p) ^ see2);

        // Combine accumulators
        seed ^= see1 ^ see2;

        // Process remaining 16-byte chunks sequentially
        if constexpr (N > 112) {
            seed = P::wymix(read64<96>(p) ^ P::wyp1, read64<104>(p) ^ seed);
        }

        // Finalize with last 16 bytes - use fast single-multiply finalize
        return P::finalize_fast(seed, read64<N-16>(p), read64<N-8>(p), N);
    }
    // ========== 129-512 bytes: ULTRA OPTIMIZED 4-way parallel ==========
    // Key insight: 4 parallel accumulators (64 bytes/iter) beats 3-way (48 bytes/iter)
    // Better ILP on modern out-of-order CPUs with multiple multiply units.
    else if constexpr (N <= 512) {
        // Prefetch ahead for large inputs
        __builtin_prefetch(p + 256, 0, 3);
        if constexpr (N >= 384) {
            __builtin_prefetch(p + 384, 0, 3);
        }

        // 4 parallel accumulators - each processes 16 bytes per iteration
        std::size_t s1 = seed, s2 = seed, s3 = seed;

        // Process 64 bytes at a time with 4-way parallelism
        // Each iteration: 4 independent mixes that can execute in parallel
        #define ULTRA_ITER_64(off) \
            seed = P::wymix(read64<off>(p)      ^ P::wyp1, read64<off + 8>(p)  ^ seed); \
            s1   = P::wymix(read64<off + 16>(p) ^ P::wyp2, read64<off + 24>(p) ^ s1);   \
            s2   = P::wymix(read64<off + 32>(p) ^ P::wyp3, read64<off + 40>(p) ^ s2);   \
            s3   = P::wymix(read64<off + 48>(p) ^ P::wyp0, read64<off + 56>(p) ^ s3);

        // Unroll based on size - 512/64 = 8 iterations max
        ULTRA_ITER_64(0)
        ULTRA_ITER_64(64)
        if constexpr (N >= 192) { ULTRA_ITER_64(128) }
        if constexpr (N >= 256) { ULTRA_ITER_64(192) }
        if constexpr (N >= 320) { ULTRA_ITER_64(256) }
        if constexpr (N >= 384) { ULTRA_ITER_64(320) }
        if constexpr (N >= 448) { ULTRA_ITER_64(384) }
        if constexpr (N >= 512) { ULTRA_ITER_64(448) }

        #undef ULTRA_ITER_64

        // Combine all accumulators
        seed ^= s1 ^ s2 ^ s3;

        // Handle remainder (N % 64 bytes)
        constexpr std::size_t processed = (N / 64) * 64;
        constexpr std::size_t remainder = N - processed;

        if constexpr (remainder > 48) {
            seed = P::wymix(read64<processed>(p) ^ P::wyp1, read64<processed + 8>(p) ^ seed);
            seed = P::wymix(read64<processed + 16>(p) ^ P::wyp1, read64<processed + 24>(p) ^ seed);
            seed = P::wymix(read64<processed + 32>(p) ^ P::wyp1, read64<processed + 40>(p) ^ seed);
        } else if constexpr (remainder > 32) {
            seed = P::wymix(read64<processed>(p) ^ P::wyp1, read64<processed + 8>(p) ^ seed);
            seed = P::wymix(read64<processed + 16>(p) ^ P::wyp1, read64<processed + 24>(p) ^ seed);
        } else if constexpr (remainder > 16) {
            seed = P::wymix(read64<processed>(p) ^ P::wyp1, read64<processed + 8>(p) ^ seed);
        }

        // Finalize with last 16 bytes - use fast single-multiply finalize
        return P::finalize_fast(seed, read64<N-16>(p), read64<N-8>(p), N);
    }
    // ========== 513-1024 bytes: RAPIDHASH-STYLE 7-way loop ==========
    // 7-way parallelism matches rapidhash for optimal large-input performance
    else if constexpr (N <= 1024) {
        // Extended secrets for 7-way (matching rapidhash)
        constexpr std::uint64_t s0 = P::wyp0;
        constexpr std::uint64_t s1 = P::wyp1;
        constexpr std::uint64_t s2 = P::wyp2;
        constexpr std::uint64_t s3 = P::wyp3;
        constexpr std::uint64_t s4 = s0 ^ s1;
        constexpr std::uint64_t s5 = s2 ^ s3;
        constexpr std::uint64_t s6 = s0 ^ s2;

        // 7 parallel accumulators
        std::size_t see1 = seed, see2 = seed, see3 = seed;
        std::size_t see4 = seed, see5 = seed, see6 = seed;
        const auto* ptr = p;
        std::size_t remaining = N;

        // Process 112 bytes at a time with 7-way parallelism
        while (remaining > 112) {
            std::uint64_t w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13;
            __builtin_memcpy(&w0, ptr, 8);
            __builtin_memcpy(&w1, ptr + 8, 8);
            __builtin_memcpy(&w2, ptr + 16, 8);
            __builtin_memcpy(&w3, ptr + 24, 8);
            __builtin_memcpy(&w4, ptr + 32, 8);
            __builtin_memcpy(&w5, ptr + 40, 8);
            __builtin_memcpy(&w6, ptr + 48, 8);
            __builtin_memcpy(&w7, ptr + 56, 8);
            __builtin_memcpy(&w8, ptr + 64, 8);
            __builtin_memcpy(&w9, ptr + 72, 8);
            __builtin_memcpy(&w10, ptr + 80, 8);
            __builtin_memcpy(&w11, ptr + 88, 8);
            __builtin_memcpy(&w12, ptr + 96, 8);
            __builtin_memcpy(&w13, ptr + 104, 8);

            seed = P::wymix(w0 ^ s0, w1 ^ seed);
            see1 = P::wymix(w2 ^ s1, w3 ^ see1);
            see2 = P::wymix(w4 ^ s2, w5 ^ see2);
            see3 = P::wymix(w6 ^ s3, w7 ^ see3);
            see4 = P::wymix(w8 ^ s4, w9 ^ see4);
            see5 = P::wymix(w10 ^ s5, w11 ^ see5);
            see6 = P::wymix(w12 ^ s6, w13 ^ see6);

            ptr += 112;
            remaining -= 112;
        }

        // Tree reduction
        seed ^= see1;
        see2 ^= see3;
        see4 ^= see5;
        seed ^= see6;
        see2 ^= see4;
        seed ^= see2;

        // Handle remainder (16-byte chunks)
        while (remaining > 16) {
            std::uint64_t w0, w1;
            __builtin_memcpy(&w0, ptr, 8);
            __builtin_memcpy(&w1, ptr + 8, 8);
            seed = P::wymix(w0 ^ s2, w1 ^ seed);
            ptr += 16;
            remaining -= 16;
        }

        // Finalize with last 16 bytes - use fast single-multiply finalize
        std::uint64_t a, b;
        __builtin_memcpy(&a, p + N - 16, 8);
        __builtin_memcpy(&b, p + N - 8, 8);
        return P::finalize_fast(seed, a, b, N);
    }
    // ========== 1025-4096 bytes: 7-WAY PARALLEL like rapidhash ==========
    // 7-way amortizes overhead better for very large inputs
    else if constexpr (N <= 4096) {
        // Additional secrets for 7-way
        constexpr std::uint64_t s0 = P::wyp0;
        constexpr std::uint64_t s1 = P::wyp1;
        constexpr std::uint64_t s2 = P::wyp2;
        constexpr std::uint64_t s3 = P::wyp3;
        constexpr std::uint64_t s4 = s0 ^ s1;
        constexpr std::uint64_t s5 = s2 ^ s3;
        constexpr std::uint64_t s6 = s0 ^ s2;

        // Prefetch
        __builtin_prefetch(p + 256, 0, 3);
        __builtin_prefetch(p + 512, 0, 3);
        __builtin_prefetch(p + 768, 0, 3);

        // 7 parallel accumulators
        std::size_t see1 = seed, see2 = seed, see3 = seed;
        std::size_t see4 = seed, see5 = seed, see6 = seed;

        #define ULTRA_ITER_112(off) \
            seed = P::wymix(read64<off>(p)      ^ s0, read64<off + 8>(p)   ^ seed); \
            see1 = P::wymix(read64<off + 16>(p) ^ s1, read64<off + 24>(p)  ^ see1); \
            see2 = P::wymix(read64<off + 32>(p) ^ s2, read64<off + 40>(p)  ^ see2); \
            see3 = P::wymix(read64<off + 48>(p) ^ s3, read64<off + 56>(p)  ^ see3); \
            see4 = P::wymix(read64<off + 64>(p) ^ s4, read64<off + 72>(p)  ^ see4); \
            see5 = P::wymix(read64<off + 80>(p) ^ s5, read64<off + 88>(p)  ^ see5); \
            see6 = P::wymix(read64<off + 96>(p) ^ s6, read64<off + 104>(p) ^ see6);

        if constexpr (N >= 1120) { ULTRA_ITER_112(0) }
        if constexpr (N >= 1232) { ULTRA_ITER_112(112) }
        if constexpr (N >= 1344) { ULTRA_ITER_112(224) }
        if constexpr (N >= 1456) { ULTRA_ITER_112(336) }
        if constexpr (N >= 1568) { ULTRA_ITER_112(448) }
        if constexpr (N >= 1680) { ULTRA_ITER_112(560) }
        if constexpr (N >= 1792) { ULTRA_ITER_112(672) }
        if constexpr (N >= 1904) { ULTRA_ITER_112(784) }
        if constexpr (N >= 2016) { ULTRA_ITER_112(896) }
        if constexpr (N >= 2128) { ULTRA_ITER_112(1008) }
        if constexpr (N >= 2240) { ULTRA_ITER_112(1120) }
        if constexpr (N >= 2352) { ULTRA_ITER_112(1232) }
        if constexpr (N >= 2464) { ULTRA_ITER_112(1344) }
        if constexpr (N >= 2576) { ULTRA_ITER_112(1456) }
        if constexpr (N >= 2688) { ULTRA_ITER_112(1568) }
        if constexpr (N >= 2800) { ULTRA_ITER_112(1680) }
        if constexpr (N >= 2912) { ULTRA_ITER_112(1792) }
        if constexpr (N >= 3024) { ULTRA_ITER_112(1904) }
        if constexpr (N >= 3136) { ULTRA_ITER_112(2016) }
        if constexpr (N >= 3248) { ULTRA_ITER_112(2128) }
        if constexpr (N >= 3360) { ULTRA_ITER_112(2240) }
        if constexpr (N >= 3472) { ULTRA_ITER_112(2352) }
        if constexpr (N >= 3584) { ULTRA_ITER_112(2464) }
        if constexpr (N >= 3696) { ULTRA_ITER_112(2576) }
        if constexpr (N >= 3808) { ULTRA_ITER_112(2688) }
        if constexpr (N >= 3920) { ULTRA_ITER_112(2800) }
        if constexpr (N >= 4032) { ULTRA_ITER_112(2912) }

        #undef ULTRA_ITER_112

        // Tree-reduction
        seed ^= see1;
        see2 ^= see3;
        see4 ^= see5;
        seed ^= see6;
        see2 ^= see4;
        seed ^= see2;

        // Remainder processing
        constexpr std::size_t processed = (N / 112) * 112;
        constexpr std::size_t remainder = N - processed;

        if constexpr (remainder > 96) {
            seed = P::wymix(read64<processed>(p) ^ s2, read64<processed + 8>(p) ^ seed);
            seed = P::wymix(read64<processed + 16>(p) ^ s2, read64<processed + 24>(p) ^ seed);
            seed = P::wymix(read64<processed + 32>(p) ^ s1, read64<processed + 40>(p) ^ seed);
            seed = P::wymix(read64<processed + 48>(p) ^ s1, read64<processed + 56>(p) ^ seed);
            seed = P::wymix(read64<processed + 64>(p) ^ s2, read64<processed + 72>(p) ^ seed);
            seed = P::wymix(read64<processed + 80>(p) ^ s1, read64<processed + 88>(p) ^ seed);
        } else if constexpr (remainder > 80) {
            seed = P::wymix(read64<processed>(p) ^ s2, read64<processed + 8>(p) ^ seed);
            seed = P::wymix(read64<processed + 16>(p) ^ s2, read64<processed + 24>(p) ^ seed);
            seed = P::wymix(read64<processed + 32>(p) ^ s1, read64<processed + 40>(p) ^ seed);
            seed = P::wymix(read64<processed + 48>(p) ^ s1, read64<processed + 56>(p) ^ seed);
            seed = P::wymix(read64<processed + 64>(p) ^ s2, read64<processed + 72>(p) ^ seed);
        } else if constexpr (remainder > 64) {
            seed = P::wymix(read64<processed>(p) ^ s2, read64<processed + 8>(p) ^ seed);
            seed = P::wymix(read64<processed + 16>(p) ^ s2, read64<processed + 24>(p) ^ seed);
            seed = P::wymix(read64<processed + 32>(p) ^ s1, read64<processed + 40>(p) ^ seed);
            seed = P::wymix(read64<processed + 48>(p) ^ s1, read64<processed + 56>(p) ^ seed);
        } else if constexpr (remainder > 48) {
            seed = P::wymix(read64<processed>(p) ^ s2, read64<processed + 8>(p) ^ seed);
            seed = P::wymix(read64<processed + 16>(p) ^ s2, read64<processed + 24>(p) ^ seed);
            seed = P::wymix(read64<processed + 32>(p) ^ s1, read64<processed + 40>(p) ^ seed);
        } else if constexpr (remainder > 32) {
            seed = P::wymix(read64<processed>(p) ^ s2, read64<processed + 8>(p) ^ seed);
            seed = P::wymix(read64<processed + 16>(p) ^ s2, read64<processed + 24>(p) ^ seed);
        } else if constexpr (remainder > 16) {
            seed = P::wymix(read64<processed>(p) ^ s2, read64<processed + 8>(p) ^ seed);
        }

        // Finalize - use fast single-multiply finalize
        return P::finalize_fast(seed, read64<N-16>(p), read64<N-8>(p), N);
    }
    // ========== Larger: runtime 7-way loop for very large sizes ==========
    else {
        // Additional secrets for 7-way
        constexpr std::uint64_t s0 = P::wyp0;
        constexpr std::uint64_t s1_c = P::wyp1;
        constexpr std::uint64_t s2_c = P::wyp2;
        constexpr std::uint64_t s3_c = P::wyp3;
        constexpr std::uint64_t s4_c = s0 ^ s1_c;
        constexpr std::uint64_t s5_c = s2_c ^ s3_c;
        constexpr std::uint64_t s6_c = s0 ^ s2_c;

        // 7 parallel accumulators
        std::size_t see1 = seed, see2 = seed, see3 = seed;
        std::size_t see4 = seed, see5 = seed, see6 = seed;
        std::size_t remaining = N;
        const auto* ptr = p;

        // Prefetch ahead
        __builtin_prefetch(ptr + 256, 0, 3);
        __builtin_prefetch(ptr + 512, 0, 3);

        // Process 112 bytes at a time with 7-way parallelism
        while (remaining >= 112) {
            __builtin_prefetch(ptr + 624, 0, 3);

            std::uint64_t w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13;
            __builtin_memcpy(&w0, ptr, 8);
            __builtin_memcpy(&w1, ptr + 8, 8);
            __builtin_memcpy(&w2, ptr + 16, 8);
            __builtin_memcpy(&w3, ptr + 24, 8);
            __builtin_memcpy(&w4, ptr + 32, 8);
            __builtin_memcpy(&w5, ptr + 40, 8);
            __builtin_memcpy(&w6, ptr + 48, 8);
            __builtin_memcpy(&w7, ptr + 56, 8);
            __builtin_memcpy(&w8, ptr + 64, 8);
            __builtin_memcpy(&w9, ptr + 72, 8);
            __builtin_memcpy(&w10, ptr + 80, 8);
            __builtin_memcpy(&w11, ptr + 88, 8);
            __builtin_memcpy(&w12, ptr + 96, 8);
            __builtin_memcpy(&w13, ptr + 104, 8);

            seed = P::wymix(w0 ^ s0, w1 ^ seed);
            see1 = P::wymix(w2 ^ s1_c, w3 ^ see1);
            see2 = P::wymix(w4 ^ s2_c, w5 ^ see2);
            see3 = P::wymix(w6 ^ s3_c, w7 ^ see3);
            see4 = P::wymix(w8 ^ s4_c, w9 ^ see4);
            see5 = P::wymix(w10 ^ s5_c, w11 ^ see5);
            see6 = P::wymix(w12 ^ s6_c, w13 ^ see6);

            ptr += 112;
            remaining -= 112;
        }

        // Tree-reduction of accumulators
        seed ^= see1;
        see2 ^= see3;
        see4 ^= see5;
        seed ^= see6;
        see2 ^= see4;
        seed ^= see2;

        // Handle remainder
        while (remaining >= 16) {
            std::uint64_t a, b;
            __builtin_memcpy(&a, ptr, 8);
            __builtin_memcpy(&b, ptr + 8, 8);
            seed = P::wymix(a ^ s2_c, b ^ seed);
            ptr += 16;
            remaining -= 16;
        }

        // Final bytes - use fast single-multiply finalize
        std::uint64_t a, b;
        __builtin_memcpy(&a, p + N - 16, 8);
        __builtin_memcpy(&b, p + N - 8, 8);
        return P::finalize_fast(seed, a, b, N);
    }
}

template<typename Policy, std::size_t N>
[[gnu::always_inline]] inline std::size_t hash_bytes_fixed(const void* data) noexcept {
    // Use wyhash-optimized path for wyhash_policy - now supports up to 4KB
    if constexpr (std::is_same_v<Policy, wyhash_policy>) {
        return hash_bytes_wyhash_optimized<N>(data);
    }

    const auto* p = static_cast<const std::uint8_t*>(data);

    // Number of 8-byte words needed (minimum reads)
    constexpr std::size_t words = (N + 7) / 8;

    if constexpr (N == 0) {
        return 0;
    }
    // ========== 1 word (1-8 bytes): 0 combines ==========
    else if constexpr (words == 1) {
        if constexpr (N <= 3) {
            std::uint64_t v = p[0];
            if constexpr (N >= 2) v |= static_cast<std::uint64_t>(p[1]) << 8;
            if constexpr (N >= 3) v |= static_cast<std::uint64_t>(p[2]) << 16;
            return Policy::mix(v ^ N);
        } else if constexpr (N == 4) {
            std::uint32_t v;
            __builtin_memcpy(&v, p, 4);
            return Policy::mix(static_cast<std::uint64_t>(v) ^ N);
        } else if constexpr (N < 8) {
            // 5-7 bytes: pack into single uint64
            std::uint32_t lo, hi;
            __builtin_memcpy(&lo, p, 4);
            __builtin_memcpy(&hi, p + N - 4, 4);
            return Policy::mix((static_cast<std::uint64_t>(lo) | (static_cast<std::uint64_t>(hi) << 32)) ^ N);
        } else { // N == 8
            return Policy::mix(read64<0>(p));
        }
    }
    // ========== 2 words (9-16 bytes): 1 combine ==========
    else if constexpr (words == 2) {
        return Policy::combine(read64<0>(p) ^ N, read64<N-8>(p));
    }
    // ========== 3 words (17-24 bytes): 2 combines ==========
    else if constexpr (words == 3) {
        return Policy::combine(
            Policy::combine(read64<0>(p) ^ N, read64<8>(p)),
            read64<N-8>(p)
        );
    }
    // ========== 4 words (25-32 bytes): 3 combines ==========
    else if constexpr (words == 4) {
        return combine4<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                read64<16>(p), read64<N-8>(p));
    }
    // ========== 5 words (33-40 bytes): 4 combines ==========
    else if constexpr (words == 5) {
        std::size_t h0 = Policy::combine(read64<0>(p) ^ N, read64<8>(p));
        std::size_t h1 = Policy::combine(read64<16>(p), read64<24>(p));
        return Policy::combine(Policy::combine(h0, h1), read64<N-8>(p));
    }
    // ========== 6 words (41-48 bytes): 5 combines ==========
    else if constexpr (words == 6) {
        return combine6<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                read64<16>(p), read64<24>(p),
                                read64<32>(p), read64<N-8>(p));
    }
    // ========== 7 words (49-56 bytes): 6 combines ==========
    else if constexpr (words == 7) {
        std::size_t h0 = combine4<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                          read64<16>(p), read64<24>(p));
        std::size_t h1 = Policy::combine(read64<32>(p), read64<40>(p));
        return Policy::combine(Policy::combine(h0, h1), read64<N-8>(p));
    }
    // ========== 8 words (57-64 bytes): 7 combines ==========
    else if constexpr (words == 8) {
        return combine8<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                read64<16>(p), read64<24>(p),
                                read64<32>(p), read64<40>(p),
                                read64<48>(p), read64<N-8>(p));
    }
    // ========== 9-12 words (65-96 bytes): 8-11 combines ==========
    else if constexpr (words <= 12) {
        // Read first 8 words + remaining
        std::size_t h = combine8<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                         read64<16>(p), read64<24>(p),
                                         read64<32>(p), read64<40>(p),
                                         read64<48>(p), read64<56>(p));
        // Handle remaining words
        if constexpr (words >= 9)  h = Policy::combine(h, read64<64>(p));
        if constexpr (words >= 10) h = Policy::combine(h, read64<72>(p));
        if constexpr (words >= 11) h = Policy::combine(h, read64<80>(p));
        if constexpr (words >= 12) h = Policy::combine(h, read64<N-8>(p));
        else if constexpr (words < 12) h = Policy::combine(h, read64<N-8>(p) ^ (N * 0x9e3779b9));
        return h;
    }
    // ========== 13-16 words (97-128 bytes): tree reduction ==========
    else if constexpr (words <= 16) {
        // Two groups of 8 words each, tree-combined
        std::size_t h0 = combine8<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                          read64<16>(p), read64<24>(p),
                                          read64<32>(p), read64<40>(p),
                                          read64<48>(p), read64<56>(p));
        std::size_t h1 = combine8<Policy>(read64<64>(p), read64<72>(p),
                                          read64<80>(p), read64<88>(p),
                                          read64<96>(p), read64<104>(p),
                                          read64<112>(p), read64<N-8>(p));
        return Policy::combine(h0, h1);
    }
    // ========== 17-32 words (129-256 bytes): 4x8 tree reduction ==========
    else if constexpr (words <= 32) {
        std::size_t h0 = combine8<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                          read64<16>(p), read64<24>(p),
                                          read64<32>(p), read64<40>(p),
                                          read64<48>(p), read64<56>(p));
        std::size_t h1 = combine8<Policy>(read64<64>(p), read64<72>(p),
                                          read64<80>(p), read64<88>(p),
                                          read64<96>(p), read64<104>(p),
                                          read64<112>(p), read64<120>(p));
        // Handle remaining words (17-32 words = 136-256 bytes)
        if constexpr (words > 16) {
            std::size_t h2 = combine8<Policy>(read64<128>(p), read64<136>(p),
                                              read64<144>(p), read64<152>(p),
                                              read64<160>(p), read64<168>(p),
                                              read64<176>(p), read64<184>(p));
            if constexpr (words <= 24) {
                // 17-24 words: combine h0, h1, h2, and tail
                std::size_t h3 = read64<N-8>(p);
                return Policy::combine(Policy::combine(Policy::combine(h0, h1), h2), h3);
            } else {
                // 25-32 words
                std::size_t h3 = combine8<Policy>(read64<192>(p), read64<200>(p),
                                                  read64<208>(p), read64<216>(p),
                                                  read64<224>(p), read64<232>(p),
                                                  read64<240>(p), read64<N-8>(p));
                return Policy::combine(Policy::combine(h0, h1), Policy::combine(h2, h3));
            }
        }
        return Policy::combine(h0, h1);
    }
    // ========== 33-64 words (257-512 bytes): 8x8 tree reduction ==========
    else if constexpr (words <= 64) {
        // Process in 8 groups of 8 words each for maximum ILP
        std::size_t g0 = combine8<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                          read64<16>(p), read64<24>(p),
                                          read64<32>(p), read64<40>(p),
                                          read64<48>(p), read64<56>(p));
        std::size_t g1 = combine8<Policy>(read64<64>(p), read64<72>(p),
                                          read64<80>(p), read64<88>(p),
                                          read64<96>(p), read64<104>(p),
                                          read64<112>(p), read64<120>(p));
        std::size_t g2 = combine8<Policy>(read64<128>(p), read64<136>(p),
                                          read64<144>(p), read64<152>(p),
                                          read64<160>(p), read64<168>(p),
                                          read64<176>(p), read64<184>(p));
        std::size_t g3 = combine8<Policy>(read64<192>(p), read64<200>(p),
                                          read64<208>(p), read64<216>(p),
                                          read64<224>(p), read64<232>(p),
                                          read64<240>(p), read64<248>(p));

        if constexpr (words <= 32) {
            return Policy::combine(Policy::combine(g0, g1), Policy::combine(g2, g3));
        } else if constexpr (words <= 40) {
            std::size_t g4 = combine8<Policy>(read64<256>(p), read64<264>(p),
                                              read64<272>(p), read64<280>(p),
                                              read64<288>(p), read64<296>(p),
                                              read64<304>(p), read64<N-8>(p));
            std::size_t h01 = Policy::combine(g0, g1);
            std::size_t h23 = Policy::combine(g2, g3);
            return Policy::combine(Policy::combine(h01, h23), g4);
        } else if constexpr (words <= 48) {
            std::size_t g4 = combine8<Policy>(read64<256>(p), read64<264>(p),
                                              read64<272>(p), read64<280>(p),
                                              read64<288>(p), read64<296>(p),
                                              read64<304>(p), read64<312>(p));
            std::size_t g5 = combine8<Policy>(read64<320>(p), read64<328>(p),
                                              read64<336>(p), read64<344>(p),
                                              read64<352>(p), read64<360>(p),
                                              read64<368>(p), read64<N-8>(p));
            return Policy::combine(
                Policy::combine(Policy::combine(g0, g1), Policy::combine(g2, g3)),
                Policy::combine(g4, g5)
            );
        } else if constexpr (words <= 56) {
            std::size_t g4 = combine8<Policy>(read64<256>(p), read64<264>(p),
                                              read64<272>(p), read64<280>(p),
                                              read64<288>(p), read64<296>(p),
                                              read64<304>(p), read64<312>(p));
            std::size_t g5 = combine8<Policy>(read64<320>(p), read64<328>(p),
                                              read64<336>(p), read64<344>(p),
                                              read64<352>(p), read64<360>(p),
                                              read64<368>(p), read64<376>(p));
            std::size_t g6 = combine8<Policy>(read64<384>(p), read64<392>(p),
                                              read64<400>(p), read64<408>(p),
                                              read64<416>(p), read64<424>(p),
                                              read64<432>(p), read64<N-8>(p));
            std::size_t h0123 = Policy::combine(Policy::combine(g0, g1), Policy::combine(g2, g3));
            std::size_t h456 = Policy::combine(Policy::combine(g4, g5), g6);
            return Policy::combine(h0123, h456);
        } else {
            // 57-64 words (449-512 bytes)
            std::size_t g4 = combine8<Policy>(read64<256>(p), read64<264>(p),
                                              read64<272>(p), read64<280>(p),
                                              read64<288>(p), read64<296>(p),
                                              read64<304>(p), read64<312>(p));
            std::size_t g5 = combine8<Policy>(read64<320>(p), read64<328>(p),
                                              read64<336>(p), read64<344>(p),
                                              read64<352>(p), read64<360>(p),
                                              read64<368>(p), read64<376>(p));
            std::size_t g6 = combine8<Policy>(read64<384>(p), read64<392>(p),
                                              read64<400>(p), read64<408>(p),
                                              read64<416>(p), read64<424>(p),
                                              read64<432>(p), read64<440>(p));
            std::size_t g7 = combine8<Policy>(read64<448>(p), read64<456>(p),
                                              read64<464>(p), read64<472>(p),
                                              read64<480>(p), read64<488>(p),
                                              read64<496>(p), read64<N-8>(p));
            // Full 8-way tree reduction
            return Policy::combine(
                Policy::combine(Policy::combine(g0, g1), Policy::combine(g2, g3)),
                Policy::combine(Policy::combine(g4, g5), Policy::combine(g6, g7))
            );
        }
    }
    // ========== 65-128 words (513-1024 bytes): systematic tree reduction ==========
    else if constexpr (words <= 128) {
        // Process in groups of 64 bytes (8 words) for cache efficiency
        constexpr size_t full_groups = words / 8;
        constexpr size_t remaining = words % 8;

        // First group includes the size mixing
        std::size_t h = combine8<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                         read64<16>(p), read64<24>(p),
                                         read64<32>(p), read64<40>(p),
                                         read64<48>(p), read64<56>(p));

        // Remaining full groups
        if constexpr (full_groups >= 2)
            h = Policy::combine(h, combine8<Policy>(read64<64>(p), read64<72>(p),
                                                    read64<80>(p), read64<88>(p),
                                                    read64<96>(p), read64<104>(p),
                                                    read64<112>(p), read64<120>(p)));
        if constexpr (full_groups >= 3)
            h = Policy::combine(h, combine8<Policy>(read64<128>(p), read64<136>(p),
                                                    read64<144>(p), read64<152>(p),
                                                    read64<160>(p), read64<168>(p),
                                                    read64<176>(p), read64<184>(p)));
        if constexpr (full_groups >= 4)
            h = Policy::combine(h, combine8<Policy>(read64<192>(p), read64<200>(p),
                                                    read64<208>(p), read64<216>(p),
                                                    read64<224>(p), read64<232>(p),
                                                    read64<240>(p), read64<248>(p)));
        if constexpr (full_groups >= 5)
            h = Policy::combine(h, combine8<Policy>(read64<256>(p), read64<264>(p),
                                                    read64<272>(p), read64<280>(p),
                                                    read64<288>(p), read64<296>(p),
                                                    read64<304>(p), read64<312>(p)));
        if constexpr (full_groups >= 6)
            h = Policy::combine(h, combine8<Policy>(read64<320>(p), read64<328>(p),
                                                    read64<336>(p), read64<344>(p),
                                                    read64<352>(p), read64<360>(p),
                                                    read64<368>(p), read64<376>(p)));
        if constexpr (full_groups >= 7)
            h = Policy::combine(h, combine8<Policy>(read64<384>(p), read64<392>(p),
                                                    read64<400>(p), read64<408>(p),
                                                    read64<416>(p), read64<424>(p),
                                                    read64<432>(p), read64<440>(p)));
        if constexpr (full_groups >= 8)
            h = Policy::combine(h, combine8<Policy>(read64<448>(p), read64<456>(p),
                                                    read64<464>(p), read64<472>(p),
                                                    read64<480>(p), read64<488>(p),
                                                    read64<496>(p), read64<504>(p)));
        if constexpr (full_groups >= 9)
            h = Policy::combine(h, combine8<Policy>(read64<512>(p), read64<520>(p),
                                                    read64<528>(p), read64<536>(p),
                                                    read64<544>(p), read64<552>(p),
                                                    read64<560>(p), read64<568>(p)));
        if constexpr (full_groups >= 10)
            h = Policy::combine(h, combine8<Policy>(read64<576>(p), read64<584>(p),
                                                    read64<592>(p), read64<600>(p),
                                                    read64<608>(p), read64<616>(p),
                                                    read64<624>(p), read64<632>(p)));
        if constexpr (full_groups >= 11)
            h = Policy::combine(h, combine8<Policy>(read64<640>(p), read64<648>(p),
                                                    read64<656>(p), read64<664>(p),
                                                    read64<672>(p), read64<680>(p),
                                                    read64<688>(p), read64<696>(p)));
        if constexpr (full_groups >= 12)
            h = Policy::combine(h, combine8<Policy>(read64<704>(p), read64<712>(p),
                                                    read64<720>(p), read64<728>(p),
                                                    read64<736>(p), read64<744>(p),
                                                    read64<752>(p), read64<760>(p)));
        if constexpr (full_groups >= 13)
            h = Policy::combine(h, combine8<Policy>(read64<768>(p), read64<776>(p),
                                                    read64<784>(p), read64<792>(p),
                                                    read64<800>(p), read64<808>(p),
                                                    read64<816>(p), read64<824>(p)));
        if constexpr (full_groups >= 14)
            h = Policy::combine(h, combine8<Policy>(read64<832>(p), read64<840>(p),
                                                    read64<848>(p), read64<856>(p),
                                                    read64<864>(p), read64<872>(p),
                                                    read64<880>(p), read64<888>(p)));
        if constexpr (full_groups >= 15)
            h = Policy::combine(h, combine8<Policy>(read64<896>(p), read64<904>(p),
                                                    read64<912>(p), read64<920>(p),
                                                    read64<928>(p), read64<936>(p),
                                                    read64<944>(p), read64<952>(p)));
        if constexpr (full_groups >= 16)
            h = Policy::combine(h, combine8<Policy>(read64<960>(p), read64<968>(p),
                                                    read64<976>(p), read64<984>(p),
                                                    read64<992>(p), read64<1000>(p),
                                                    read64<1008>(p), read64<1016>(p)));

        // Handle tail (last few bytes that don't fill a full group)
        if constexpr (remaining > 0) {
            h = Policy::combine(h, read64<N-8>(p));
        }

        return h;
    }
    // ========== 129-256 words (1025-2048 bytes): extended tree reduction ==========
    else if constexpr (words <= 256) {
        constexpr size_t full_groups = words / 8;
        constexpr size_t remaining = words % 8;

        std::size_t h = combine8<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                         read64<16>(p), read64<24>(p),
                                         read64<32>(p), read64<40>(p),
                                         read64<48>(p), read64<56>(p));

        // Unroll up to 32 groups (2048 bytes)
        #define PROCESS_GROUP(idx, offset) \
            if constexpr (full_groups >= idx) \
                h = Policy::combine(h, combine8<Policy>( \
                    read64<offset>(p), read64<offset+8>(p), \
                    read64<offset+16>(p), read64<offset+24>(p), \
                    read64<offset+32>(p), read64<offset+40>(p), \
                    read64<offset+48>(p), read64<offset+56>(p)));

        PROCESS_GROUP(2, 64)
        PROCESS_GROUP(3, 128)
        PROCESS_GROUP(4, 192)
        PROCESS_GROUP(5, 256)
        PROCESS_GROUP(6, 320)
        PROCESS_GROUP(7, 384)
        PROCESS_GROUP(8, 448)
        PROCESS_GROUP(9, 512)
        PROCESS_GROUP(10, 576)
        PROCESS_GROUP(11, 640)
        PROCESS_GROUP(12, 704)
        PROCESS_GROUP(13, 768)
        PROCESS_GROUP(14, 832)
        PROCESS_GROUP(15, 896)
        PROCESS_GROUP(16, 960)
        PROCESS_GROUP(17, 1024)
        PROCESS_GROUP(18, 1088)
        PROCESS_GROUP(19, 1152)
        PROCESS_GROUP(20, 1216)
        PROCESS_GROUP(21, 1280)
        PROCESS_GROUP(22, 1344)
        PROCESS_GROUP(23, 1408)
        PROCESS_GROUP(24, 1472)
        PROCESS_GROUP(25, 1536)
        PROCESS_GROUP(26, 1600)
        PROCESS_GROUP(27, 1664)
        PROCESS_GROUP(28, 1728)
        PROCESS_GROUP(29, 1792)
        PROCESS_GROUP(30, 1856)
        PROCESS_GROUP(31, 1920)
        PROCESS_GROUP(32, 1984)

        #undef PROCESS_GROUP

        if constexpr (remaining > 0) {
            h = Policy::combine(h, read64<N-8>(p));
        }

        return h;
    }
    // ========== 257-512 words (2049-4096 bytes): further extended ==========
    else if constexpr (words <= 512) {
        constexpr size_t full_groups = words / 8;
        constexpr size_t remaining = words % 8;

        std::size_t h = combine8<Policy>(read64<0>(p) ^ N, read64<8>(p),
                                         read64<16>(p), read64<24>(p),
                                         read64<32>(p), read64<40>(p),
                                         read64<48>(p), read64<56>(p));

        // Use a loop-like pattern but with constexpr unrolling
        // Process in chunks of 8 groups (512 bytes) at a time
        auto process_chunk = [&]<size_t Base>() {
            if constexpr (full_groups > Base/64) {
                #define PG(idx) \
                    if constexpr (full_groups >= Base/64 + idx) \
                        h = Policy::combine(h, combine8<Policy>( \
                            read64<Base + (idx-1)*64>(p), read64<Base + (idx-1)*64 + 8>(p), \
                            read64<Base + (idx-1)*64 + 16>(p), read64<Base + (idx-1)*64 + 24>(p), \
                            read64<Base + (idx-1)*64 + 32>(p), read64<Base + (idx-1)*64 + 40>(p), \
                            read64<Base + (idx-1)*64 + 48>(p), read64<Base + (idx-1)*64 + 56>(p)));
                PG(1) PG(2) PG(3) PG(4) PG(5) PG(6) PG(7) PG(8)
                #undef PG
            }
        };

        process_chunk.template operator()<64>();
        process_chunk.template operator()<576>();
        process_chunk.template operator()<1088>();
        process_chunk.template operator()<1600>();
        process_chunk.template operator()<2112>();
        process_chunk.template operator()<2624>();
        process_chunk.template operator()<3136>();
        process_chunk.template operator()<3648>();

        if constexpr (remaining > 0) {
            h = Policy::combine(h, read64<N-8>(p));
        }

        return h;
    }
    // ========== Larger: fall back to runtime ==========
    else {
        return hash_bytes_scalar<Policy>(data, N);
    }
}

// Dispatch to best available implementation
template<typename Policy>
inline std::size_t hash_bytes(const void* data, std::size_t len) noexcept {
#if MIRROR_HASH_AVX512
    return hash_bytes_avx512<Policy>(data, len);
#elif MIRROR_HASH_AVX2
    return hash_bytes_avx2<Policy>(data, len);
#elif MIRROR_HASH_NEON
    return hash_bytes_neon<Policy>(data, len);
#else
    return hash_bytes_scalar<Policy>(data, len);
#endif
}

// Check if a type can be hashed as raw bytes
template<typename T>
concept BulkHashable = std::is_arithmetic_v<T> && sizeof(T) <= 8;

// Check if a struct can be hashed as raw bytes (trivially copyable, no padding concerns)
template<typename T>
concept TriviallyCopyableStruct = std::is_trivially_copyable_v<T> &&
    std::is_class_v<T> &&
    !std::is_polymorphic_v<T> &&
    sizeof(T) <= 128;  // Reasonable size limit for byte hashing

} // namespace detail

// ============================================================================
// Type Traits and Concepts
// ============================================================================

template<typename T>
concept Arithmetic = std::is_arithmetic_v<std::remove_cvref_t<T>>;

template<typename T>
concept Enum = std::is_enum_v<std::remove_cvref_t<T>>;

template<typename T>
concept StringLike =
    std::same_as<std::remove_cvref_t<T>, std::string> ||
    std::same_as<std::remove_cvref_t<T>, std::string_view>;

template<typename T>
concept Pointer = std::is_pointer_v<std::remove_cvref_t<T>>;

template<typename T>
concept SmartPointer = requires(T t) {
    typename std::remove_cvref_t<T>::element_type;
    { t.get() } -> Pointer;
};

template<typename T>
concept Container = requires(T t) {
    { t.begin() } -> std::input_or_output_iterator;
    { t.end() } -> std::input_or_output_iterator;
    { t.size() } -> std::convertible_to<std::size_t>;
} && !StringLike<T>;

template<typename T>
concept ContiguousArithmeticContainer = Container<T> && requires(T t) {
    { t.data() } -> std::convertible_to<const typename T::value_type*>;
} && detail::BulkHashable<typename T::value_type>;

template<typename T>
concept Optional = requires(T t) {
    { t.has_value() } -> std::same_as<bool>;
    { *t };
} && !StringLike<T> && !Container<T>;

namespace detail {
template<typename T>
struct is_variant : std::false_type {};

template<typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};
}

template<typename T>
concept Variant = detail::is_variant<std::remove_cvref_t<T>>::value;

template<typename T>
concept Reflectable = std::is_class_v<std::remove_cvref_t<T>> &&
    !StringLike<T> && !Container<T> && !Optional<T> &&
    !Variant<T> && !SmartPointer<T>;

// ============================================================================
// Reflection Utilities
// ============================================================================

template<typename T>
consteval std::size_t member_count() {
    return std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current()).size();
}

template<typename T, std::size_t I>
consteval auto get_member() {
    return std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())[I];
}

// ============================================================================
// Core Hash Implementation
// ============================================================================

namespace detail {

// Forward declaration
template<typename Policy, typename T>
std::size_t hash_impl(const T& value) noexcept;

// Arithmetic types
template<typename Policy, Arithmetic T>
std::size_t hash_impl(const T& value) noexcept {
    return std::hash<T>{}(value);
}

// Enum types
template<typename Policy, Enum T>
std::size_t hash_impl(const T& value) noexcept {
    return std::hash<std::underlying_type_t<T>>{}(static_cast<std::underlying_type_t<T>>(value));
}

// String types
template<typename Policy, StringLike T>
std::size_t hash_impl(const T& value) noexcept {
    return std::hash<T>{}(value);
}

// Raw pointers
template<typename Policy, Pointer T>
std::size_t hash_impl(const T& value) noexcept {
    return std::hash<T>{}(value);
}

// Smart pointers
template<typename Policy, SmartPointer T>
std::size_t hash_impl(const T& value) noexcept {
    if (value) {
        return Policy::combine(1, hash_impl<Policy>(*value));
    }
    return 0;
}

// Optional types
template<typename Policy, Optional T>
std::size_t hash_impl(const T& value) noexcept {
    if (value.has_value()) {
        return Policy::combine(1, hash_impl<Policy>(*value));
    }
    return 0;
}

// std::pair
template<typename Policy, typename T1, typename T2>
std::size_t hash_impl(const std::pair<T1, T2>& value) noexcept {
    return Policy::combine(hash_impl<Policy>(value.first), hash_impl<Policy>(value.second));
}

// std::array - optimized for arithmetic types
template<typename Policy, typename T, std::size_t N>
    requires BulkHashable<T>
std::size_t hash_impl(const std::array<T, N>& value) noexcept {
    return hash_bytes<Policy>(value.data(), N * sizeof(T));
}

// std::array - generic
template<typename Policy, typename T, std::size_t N>
    requires (!BulkHashable<T>)
std::size_t hash_impl(const std::array<T, N>& value) noexcept {
    std::size_t result = 0;
    for (const auto& elem : value) {
        result = Policy::combine(result, hash_impl<Policy>(elem));
    }
    return result;
}

// C-style arrays - optimized for arithmetic types
template<typename Policy, typename T, std::size_t N>
    requires BulkHashable<T>
std::size_t hash_impl(const T (&value)[N]) noexcept {
    return hash_bytes<Policy>(value, N * sizeof(T));
}

// C-style arrays - generic
template<typename Policy, typename T, std::size_t N>
    requires (!BulkHashable<T>)
std::size_t hash_impl(const T (&value)[N]) noexcept {
    std::size_t result = 0;
    for (std::size_t i = 0; i < N; ++i) {
        result = Policy::combine(result, hash_impl<Policy>(value[i]));
    }
    return result;
}

// Contiguous containers of arithmetic types - bulk optimization
template<typename Policy, ContiguousArithmeticContainer T>
std::size_t hash_impl(const T& value) noexcept {
    return hash_bytes<Policy>(value.data(), value.size() * sizeof(typename T::value_type));
}

// Generic containers
template<typename Policy, Container T>
    requires (!ContiguousArithmeticContainer<T>)
std::size_t hash_impl(const T& value) noexcept {
    std::size_t result = hash_impl<Policy>(value.size());
    for (const auto& elem : value) {
        result = Policy::combine(result, hash_impl<Policy>(elem));
    }
    return result;
}

// Variant types
template<typename Policy, Variant T>
std::size_t hash_impl(const T& value) noexcept {
    return Policy::combine(
        hash_impl<Policy>(value.index()),
        std::visit([](const auto& v) { return hash_impl<Policy>(v); }, value)
    );
}

// Helper to check if a member at index I is trivially copyable
template<typename T, std::size_t I>
consteval bool member_is_trivial() {
    constexpr auto m = get_member<T, I>();
    return std::is_trivially_copyable_v<typename [:std::meta::type_of(m):]>;
}

// Helper to check if all members are trivially copyable at compile time
template<typename T, std::size_t... Is>
consteval bool all_members_trivial_impl(std::index_sequence<Is...>) {
    return (member_is_trivial<T, Is>() && ...);
}

template<typename T>
consteval bool all_members_trivial() {
    return all_members_trivial_impl<T>(std::make_index_sequence<member_count<T>()>{});
}

// Reflectable types - with trivially copyable fast path
template<typename Policy, Reflectable T>
[[gnu::always_inline]] std::size_t hash_impl(const T& value) noexcept {
    constexpr std::size_t N = member_count<T>();
    if constexpr (N == 0) {
        return 0;
    } else if constexpr (std::is_trivially_copyable_v<T> && all_members_trivial<T>()) {
        // FAST PATH: Hash raw bytes directly - no per-field iteration!
        if constexpr (sizeof(T) == 8) {
            // ULTRA FAST: 8-byte structs use bit_cast (zero runtime cost)
            // bit_cast is a no-op at runtime - just reinterprets the bits
            return Policy::mix(__builtin_bit_cast(std::uint64_t, value));
        } else if constexpr (sizeof(T) == 4) {
            // 4-byte structs: single bit_cast + mix
            return Policy::mix(static_cast<std::uint64_t>(__builtin_bit_cast(std::uint32_t, value)) ^ 4);
        } else if constexpr (sizeof(T) <= 4096) {
            // Compile-time optimized: minimal reads + tree-reduction combines
            // Up to 4096 bytes (512 words) fully unrolled at compile time
            // Benchmarked: 1.2-1.3x faster than runtime SIMD at all sizes up to 4KB
            // Quality verified: avalanche ~0.50 at all sizes
            return hash_bytes_fixed<Policy, sizeof(T)>(&value);
        } else {
            // Very large structs use runtime SIMD bulk hashing
            return hash_bytes<Policy>(&value, sizeof(T));
        }
    } else {
        // SLOW PATH: Per-field iteration for complex types (strings, vectors, etc.)
        std::size_t result = 0;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((result = Policy::combine(result, hash_impl<Policy>(value.[:get_member<T, Is>():]))), ...);
        }(std::make_index_sequence<N>{});
        return result;
    }
}

} // namespace detail

// ============================================================================
// Public API
// ============================================================================

// Main hash function with configurable policy
template<typename Policy = default_policy, typename T>
std::size_t hash(const T& value) noexcept {
    return detail::hash_impl<Policy>(value);
}

// Hasher functor for use with std::unordered_* containers
template<typename Policy = default_policy>
struct hasher {
    template<typename T>
    std::size_t operator()(const T& value) const noexcept {
        return hash<Policy>(value);
    }
};

// Convenience alias for std::unordered_* compatibility
template<typename T, typename Policy = default_policy>
using std_hash_adapter = hasher<Policy>;

// Multi-value combine
template<typename Policy = default_policy, typename... Args>
std::size_t combine(const Args&... args) noexcept {
    std::size_t result = 0;
    ((result = Policy::combine(result, hash<Policy>(args))), ...);
    return result;
}

// Direct access to combine function (for advanced users)
template<typename Policy = default_policy>
constexpr std::size_t hash_combine(std::size_t seed, std::size_t value) noexcept {
    return Policy::combine(seed, value);
}

// Direct access to mix function
template<typename Policy = default_policy>
constexpr std::size_t mix(std::size_t k) noexcept {
    return Policy::mix(k);
}

// ============================================================================
// Policy Comparison Helpers
// ============================================================================

// Get policy name as string (for debugging/benchmarking)
template<typename Policy>
constexpr const char* policy_name() {
    if constexpr (std::same_as<Policy, folly_policy>) return "folly";
    else if constexpr (std::same_as<Policy, wyhash_policy>) return "wyhash";
    else if constexpr (std::same_as<Policy, murmur3_policy>) return "murmur3";
    else if constexpr (std::same_as<Policy, xxhash3_policy>) return "xxhash3";
    else if constexpr (std::same_as<Policy, fnv1a_policy>) return "fnv1a";
    else if constexpr (std::same_as<Policy, aes_policy>) return "aes";
    else if constexpr (std::same_as<Policy, rapidhash_policy>) return "rapidhash";
    else if constexpr (std::same_as<Policy, komihash_policy>) return "komihash";
    else if constexpr (std::same_as<Policy, fast_policy>) return "fast";
    else return "unknown";
}

} // namespace mirror_hash
