// ============================================================================
// STANDALONE HASH ALGORITHM BENCHMARK
// ============================================================================
// This benchmark tests the core hash algorithms without C++26 reflection.
// Can be compiled with: clang++ -std=c++20 -O3 -march=native standalone_benchmark.cpp
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <array>
#include <vector>
#include <algorithm>

// ============================================================================
// Official competitor implementations (inlined)
// ============================================================================

// wyhash v4.2 (from wangyi-fudan/wyhash)
static inline uint64_t _wyrot(uint64_t x) { return (x >> 32) | (x << 32); }

static inline uint64_t _wymum(uint64_t A, uint64_t B) {
    __uint128_t r = static_cast<__uint128_t>(A) * B;
    return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
}

static inline uint64_t _wymix(uint64_t A, uint64_t B) { return _wymum(A ^ 0x53c5ca59e6ed28a3ULL, B ^ 0x74743c1b85a1e4b3ULL); }

static inline uint64_t wyhash_read64(const uint8_t* p) {
    uint64_t v; memcpy(&v, p, 8); return v;
}
static inline uint64_t wyhash_read32(const uint8_t* p) {
    uint32_t v; memcpy(&v, p, 4); return v;
}

static inline uint64_t wyhash_official(const void* key, size_t len, uint64_t seed = 0) {
    constexpr uint64_t wyp0 = 0xa0761d6478bd642full;
    constexpr uint64_t wyp1 = 0xe7037ed1a0b428dbull;
    constexpr uint64_t wyp2 = 0x8ebc6af09c88c6e3ull;
    constexpr uint64_t wyp3 = 0x589965cc75374cc3ull;

    const uint8_t* p = static_cast<const uint8_t*>(key);
    seed ^= _wymix(seed ^ wyp0, wyp1);
    uint64_t a, b;

    if (len <= 16) {
        if (len >= 4) {
            a = (wyhash_read32(p) << 32) | wyhash_read32(p + ((len >> 3) << 2));
            b = (wyhash_read32(p + len - 4) << 32) | wyhash_read32(p + len - 4 - ((len >> 3) << 2));
        } else if (len > 0) {
            a = (static_cast<uint64_t>(p[0]) << 16) | (static_cast<uint64_t>(p[len >> 1]) << 8) | p[len - 1];
            b = 0;
        } else {
            a = b = 0;
        }
    } else {
        size_t i = len;
        if (i > 48) {
            uint64_t see1 = seed, see2 = seed;
            do {
                seed = _wymix(wyhash_read64(p) ^ wyp1, wyhash_read64(p + 8) ^ seed);
                see1 = _wymix(wyhash_read64(p + 16) ^ wyp2, wyhash_read64(p + 24) ^ see1);
                see2 = _wymix(wyhash_read64(p + 32) ^ wyp3, wyhash_read64(p + 40) ^ see2);
                p += 48; i -= 48;
            } while (i > 48);
            seed ^= see1 ^ see2;
        }
        while (i > 16) {
            seed = _wymix(wyhash_read64(p) ^ wyp1, wyhash_read64(p + 8) ^ seed);
            i -= 16; p += 16;
        }
        a = wyhash_read64(p + i - 16);
        b = wyhash_read64(p + i - 8);
    }
    a ^= wyp1; b ^= seed;
    __uint128_t r = static_cast<__uint128_t>(a) * b;
    return _wymix(static_cast<uint64_t>(r) ^ wyp0 ^ len, static_cast<uint64_t>(r >> 64) ^ wyp1);
}

// ============================================================================
// mirror_hash wyhash_policy implementation (BASELINE)
// ============================================================================

namespace baseline {

struct wyhash_policy {
    static constexpr uint64_t wyp0 = 0xa0761d6478bd642full;
    static constexpr uint64_t wyp1 = 0xe7037ed1a0b428dbull;
    static constexpr uint64_t wyp2 = 0x8ebc6af09c88c6e3ull;
    static constexpr uint64_t wyp3 = 0x589965cc75374cc3ull;
    static constexpr uint64_t INIT_SEED = 0x1ff5c2923a788d2cull;

    static constexpr uint64_t wymix(uint64_t a, uint64_t b) noexcept {
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    }

    static constexpr uint64_t finalize(uint64_t seed, uint64_t a, uint64_t b, size_t len) noexcept {
        a ^= wyp1; b ^= seed;
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        a = static_cast<uint64_t>(r); b = static_cast<uint64_t>(r >> 64);
        return wymix(a ^ wyp0 ^ len, b ^ wyp1);
    }

    static constexpr uint64_t finalize_fast(uint64_t seed, uint64_t a, uint64_t b, size_t len) noexcept {
        __uint128_t r = static_cast<__uint128_t>(a ^ wyp0 ^ len) * (b ^ wyp1 ^ seed);
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    }

    static constexpr uint64_t combine16(uint64_t seed, uint64_t a, uint64_t b) noexcept {
        return wymix(a ^ wyp1, b ^ seed);
    }
};

template<size_t N>
[[gnu::always_inline]] inline uint64_t hash_bytes_fixed(const void* data) noexcept {
    using P = wyhash_policy;
    const auto* p = static_cast<const uint8_t*>(data);
    uint64_t seed = P::INIT_SEED;

    auto read64 = [](const uint8_t* ptr) -> uint64_t {
        uint64_t v; memcpy(&v, ptr, 8); return v;
    };

    if constexpr (N == 0) {
        return 0;
    }
    else if constexpr (N <= 8) {
        if constexpr (N <= 3) {
            uint64_t v = p[0];
            if constexpr (N >= 2) v |= static_cast<uint64_t>(p[1]) << 8;
            if constexpr (N >= 3) v |= static_cast<uint64_t>(p[2]) << 16;
            return P::finalize(seed, v, 0, N);
        } else if constexpr (N == 4) {
            uint32_t v; memcpy(&v, p, 4);
            return P::finalize(seed, v, 0, N);
        } else if constexpr (N < 8) {
            uint32_t lo, hi;
            memcpy(&lo, p, 4);
            memcpy(&hi, p + N - 4, 4);
            uint64_t a = (static_cast<uint64_t>(lo) << 32) | hi;
            return P::finalize(seed, a, 0, N);
        } else {
            return P::finalize(seed, read64(p), 0, N);
        }
    }
    else if constexpr (N <= 16) {
        uint64_t a = read64(p);
        uint64_t b = read64(p + N - 8);
        a ^= P::wyp1; b ^= seed;
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        a = static_cast<uint64_t>(r); b = static_cast<uint64_t>(r >> 64);
        a ^= P::wyp0 ^ N; b ^= P::wyp1;
        r = static_cast<__uint128_t>(a) * b;
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    }
    else if constexpr (N <= 48) {
        if constexpr (N > 32) {
            seed = P::combine16(seed, read64(p), read64(p + 8));
            seed = P::combine16(seed, read64(p + 16), read64(p + 24));
        } else if constexpr (N > 16) {
            seed = P::combine16(seed, read64(p), read64(p + 8));
        }
        return P::finalize_fast(seed, read64(p + N - 16), read64(p + N - 8), N);
    }
    else if constexpr (N <= 96) {
        uint64_t w0 = read64(p), w1 = read64(p + 8);
        uint64_t w2 = read64(p + 16), w3 = read64(p + 24);
        uint64_t w4 = read64(p + 32), w5 = read64(p + 40);

        uint64_t see1 = seed, see2 = seed;
        seed = P::wymix(w0 ^ P::wyp1, w1 ^ seed);
        see1 = P::wymix(w2 ^ P::wyp2, w3 ^ see1);
        see2 = P::wymix(w4 ^ P::wyp3, w5 ^ see2);
        seed ^= see1 ^ see2;

        if constexpr (N > 64) {
            seed = P::wymix(read64(p + 48) ^ P::wyp1, read64(p + 56) ^ seed);
        }
        if constexpr (N > 80) {
            seed = P::wymix(read64(p + 64) ^ P::wyp1, read64(p + 72) ^ seed);
        }
        return P::finalize_fast(seed, read64(p + N - 16), read64(p + N - 8), N);
    }
    else if constexpr (N <= 1024) {
        constexpr uint64_t s0 = P::wyp0, s1 = P::wyp1, s2 = P::wyp2, s3 = P::wyp3;
        constexpr uint64_t s4 = s0 ^ s1, s5 = s2 ^ s3, s6 = s0 ^ s2;

        uint64_t see1 = seed, see2 = seed, see3 = seed;
        uint64_t see4 = seed, see5 = seed, see6 = seed;
        const uint8_t* ptr = p;
        size_t remaining = N;

        while (remaining > 112) {
            seed = P::wymix(read64(ptr) ^ s0, read64(ptr + 8) ^ seed);
            see1 = P::wymix(read64(ptr + 16) ^ s1, read64(ptr + 24) ^ see1);
            see2 = P::wymix(read64(ptr + 32) ^ s2, read64(ptr + 40) ^ see2);
            see3 = P::wymix(read64(ptr + 48) ^ s3, read64(ptr + 56) ^ see3);
            see4 = P::wymix(read64(ptr + 64) ^ s4, read64(ptr + 72) ^ see4);
            see5 = P::wymix(read64(ptr + 80) ^ s5, read64(ptr + 88) ^ see5);
            see6 = P::wymix(read64(ptr + 96) ^ s6, read64(ptr + 104) ^ see6);
            ptr += 112; remaining -= 112;
        }

        seed ^= see1; see2 ^= see3; see4 ^= see5;
        seed ^= see6; see2 ^= see4; seed ^= see2;

        while (remaining > 16) {
            seed = P::wymix(read64(ptr) ^ s2, read64(ptr + 8) ^ seed);
            ptr += 16; remaining -= 16;
        }

        return P::finalize_fast(seed, read64(p + N - 16), read64(p + N - 8), N);
    }
    else {
        // Fallback for very large sizes - use runtime loop
        constexpr uint64_t s0 = P::wyp0, s1 = P::wyp1, s2 = P::wyp2, s3 = P::wyp3;
        constexpr uint64_t s4 = s0 ^ s1, s5 = s2 ^ s3, s6 = s0 ^ s2;

        uint64_t see1 = seed, see2 = seed, see3 = seed;
        uint64_t see4 = seed, see5 = seed, see6 = seed;
        const uint8_t* ptr = p;
        size_t remaining = N;

        while (remaining > 112) {
            seed = P::wymix(read64(ptr) ^ s0, read64(ptr + 8) ^ seed);
            see1 = P::wymix(read64(ptr + 16) ^ s1, read64(ptr + 24) ^ see1);
            see2 = P::wymix(read64(ptr + 32) ^ s2, read64(ptr + 40) ^ see2);
            see3 = P::wymix(read64(ptr + 48) ^ s3, read64(ptr + 56) ^ see3);
            see4 = P::wymix(read64(ptr + 64) ^ s4, read64(ptr + 72) ^ see4);
            see5 = P::wymix(read64(ptr + 80) ^ s5, read64(ptr + 88) ^ see5);
            see6 = P::wymix(read64(ptr + 96) ^ s6, read64(ptr + 104) ^ see6);
            ptr += 112; remaining -= 112;
        }

        seed ^= see1; see2 ^= see3; see4 ^= see5;
        seed ^= see6; see2 ^= see4; seed ^= see2;

        while (remaining > 16) {
            seed = P::wymix(read64(ptr) ^ s2, read64(ptr + 8) ^ seed);
            ptr += 16; remaining -= 16;
        }

        return P::finalize_fast(seed, read64(p + N - 16), read64(p + N - 8), N);
    }
}

} // namespace baseline

// ============================================================================
// OPTIMIZATION 1: AES-NI Mixing Policy
// ============================================================================
#if defined(__AES__)
#include <wmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>  // SSE4.1 for _mm_extract_epi64

// Helper to extract 64-bit value from __m128i
static inline uint64_t extract_epi64(__m128i v, int i) {
    alignas(16) uint64_t vals[2];
    _mm_store_si128(reinterpret_cast<__m128i*>(vals), v);
    return vals[i];
}

namespace aes_optimized {

struct aes_policy {
    // AES round constants for mixing
    static constexpr uint64_t AES_KEY_LO = 0x243f6a8885a308d3ULL;
    static constexpr uint64_t AES_KEY_HI = 0x13198a2e03707344ULL;

    static inline __m128i aes_mix(__m128i data, __m128i key) noexcept {
        // 2 rounds of AES encryption for thorough mixing
        data = _mm_aesenc_si128(data, key);
        data = _mm_aesenc_si128(data, key);
        return data;
    }

    static inline uint64_t hash_16bytes(const void* data, __m128i key) noexcept {
        __m128i block = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));
        block = _mm_xor_si128(block, key);
        block = aes_mix(block, key);
        // Extract and fold to 64 bits
        return static_cast<uint64_t>(extract_epi64(block, 0)) ^
               static_cast<uint64_t>(extract_epi64(block, 1));
    }
};

template<size_t N>
[[gnu::always_inline]] inline uint64_t hash_bytes_aes(const void* data) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);
    __m128i key = _mm_set_epi64x(aes_policy::AES_KEY_HI, aes_policy::AES_KEY_LO);
    __m128i len_key = _mm_set_epi64x(N, N ^ 0x9e3779b97f4a7c15ULL);
    key = _mm_xor_si128(key, len_key);

    if constexpr (N == 0) {
        return 0;
    }
    else if constexpr (N < 16) {
        // Pad to 16 bytes
        alignas(16) uint8_t buf[16] = {0};
        memcpy(buf, p, N);
        return aes_policy::hash_16bytes(buf, key);
    }
    else if constexpr (N == 16) {
        return aes_policy::hash_16bytes(p, key);
    }
    else if constexpr (N <= 32) {
        __m128i acc = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        __m128i block2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + N - 16));
        acc = _mm_xor_si128(acc, key);
        acc = aes_policy::aes_mix(acc, key);
        acc = _mm_xor_si128(acc, block2);
        acc = aes_policy::aes_mix(acc, key);
        return static_cast<uint64_t>(extract_epi64(acc, 0)) ^
               static_cast<uint64_t>(extract_epi64(acc, 1));
    }
    else if constexpr (N <= 64) {
        // 4-way parallel AES
        __m128i acc0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        __m128i acc1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 16));
        __m128i acc2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 32));
        __m128i acc3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + N - 16));

        acc0 = _mm_xor_si128(acc0, key);
        acc1 = _mm_xor_si128(acc1, _mm_xor_si128(key, _mm_set_epi64x(1, 1)));
        acc2 = _mm_xor_si128(acc2, _mm_xor_si128(key, _mm_set_epi64x(2, 2)));
        acc3 = _mm_xor_si128(acc3, _mm_xor_si128(key, _mm_set_epi64x(3, 3)));

        acc0 = _mm_aesenc_si128(acc0, key);
        acc1 = _mm_aesenc_si128(acc1, key);
        acc2 = _mm_aesenc_si128(acc2, key);
        acc3 = _mm_aesenc_si128(acc3, key);

        acc0 = _mm_aesenc_si128(acc0, key);
        acc1 = _mm_aesenc_si128(acc1, key);
        acc2 = _mm_aesenc_si128(acc2, key);
        acc3 = _mm_aesenc_si128(acc3, key);

        // Combine
        acc0 = _mm_xor_si128(acc0, acc1);
        acc2 = _mm_xor_si128(acc2, acc3);
        acc0 = _mm_xor_si128(acc0, acc2);
        acc0 = _mm_aesenc_si128(acc0, key);

        return static_cast<uint64_t>(extract_epi64(acc0, 0)) ^
               static_cast<uint64_t>(extract_epi64(acc0, 1));
    }
    else {
        // For large inputs: 4-way parallel processing
        __m128i acc0 = _mm_xor_si128(_mm_set_epi64x(N, 0), key);
        __m128i acc1 = _mm_xor_si128(_mm_set_epi64x(N, 1), key);
        __m128i acc2 = _mm_xor_si128(_mm_set_epi64x(N, 2), key);
        __m128i acc3 = _mm_xor_si128(_mm_set_epi64x(N, 3), key);

        size_t i = 0;
        for (; i + 64 <= N; i += 64) {
            __m128i d0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + i));
            __m128i d1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + i + 16));
            __m128i d2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + i + 32));
            __m128i d3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + i + 48));

            acc0 = _mm_aesenc_si128(_mm_xor_si128(acc0, d0), key);
            acc1 = _mm_aesenc_si128(_mm_xor_si128(acc1, d1), key);
            acc2 = _mm_aesenc_si128(_mm_xor_si128(acc2, d2), key);
            acc3 = _mm_aesenc_si128(_mm_xor_si128(acc3, d3), key);
        }

        // Handle remainder
        for (; i + 16 <= N; i += 16) {
            __m128i d = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + i));
            acc0 = _mm_aesenc_si128(_mm_xor_si128(acc0, d), key);
        }

        if (i < N) {
            alignas(16) uint8_t buf[16] = {0};
            memcpy(buf, p + i, N - i);
            __m128i d = _mm_load_si128(reinterpret_cast<const __m128i*>(buf));
            acc0 = _mm_aesenc_si128(_mm_xor_si128(acc0, d), key);
        }

        // Final reduction
        acc0 = _mm_xor_si128(acc0, acc1);
        acc2 = _mm_xor_si128(acc2, acc3);
        acc0 = _mm_xor_si128(acc0, acc2);
        acc0 = _mm_aesenc_si128(acc0, key);
        acc0 = _mm_aesenc_si128(acc0, key);

        return static_cast<uint64_t>(extract_epi64(acc0, 0)) ^
               static_cast<uint64_t>(extract_epi64(acc0, 1));
    }
}

} // namespace aes_optimized
#endif

// ============================================================================
// OPTIMIZATION 2: 128-bit State Accumulation
// ============================================================================

namespace state128_optimized {

struct wyhash_128state_policy {
    static constexpr uint64_t wyp0 = 0xa0761d6478bd642full;
    static constexpr uint64_t wyp1 = 0xe7037ed1a0b428dbull;
    static constexpr uint64_t wyp2 = 0x8ebc6af09c88c6e3ull;
    static constexpr uint64_t wyp3 = 0x589965cc75374cc3ull;
    static constexpr uint64_t INIT_SEED = 0x1ff5c2923a788d2cull;

    // Return full 128-bit result without folding
    static inline void wymul128(uint64_t a, uint64_t b, uint64_t& lo, uint64_t& hi) noexcept {
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        lo = static_cast<uint64_t>(r);
        hi = static_cast<uint64_t>(r >> 64);
    }

    // Accumulate into 128-bit state
    static inline void accumulate(uint64_t& lo, uint64_t& hi, uint64_t a, uint64_t b, uint64_t secret) noexcept {
        uint64_t mul_lo, mul_hi;
        wymul128(a ^ secret, b, mul_lo, mul_hi);
        lo += mul_lo;
        hi ^= mul_hi;
    }

    // Final fold
    static inline uint64_t fold128(uint64_t lo, uint64_t hi, size_t len) noexcept {
        __uint128_t r = static_cast<__uint128_t>(lo ^ wyp0 ^ len) * (hi ^ wyp1);
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    }
};

template<size_t N>
[[gnu::always_inline]] inline uint64_t hash_bytes_128state(const void* data) noexcept {
    using P = wyhash_128state_policy;
    const auto* p = static_cast<const uint8_t*>(data);

    auto read64 = [](const uint8_t* ptr) -> uint64_t {
        uint64_t v; memcpy(&v, ptr, 8); return v;
    };

    // For small inputs, use baseline (128-bit state doesn't help)
    if constexpr (N <= 48) {
        return baseline::hash_bytes_fixed<N>(data);
    }
    else {
        // Use 128-bit accumulation for larger inputs
        uint64_t lo = P::INIT_SEED, hi = P::INIT_SEED;

        size_t i = 0;
        // Process 32 bytes at a time (2x 16-byte chunks)
        for (; i + 32 <= N; i += 32) {
            uint64_t a0 = read64(p + i), b0 = read64(p + i + 8);
            uint64_t a1 = read64(p + i + 16), b1 = read64(p + i + 24);

            P::accumulate(lo, hi, a0, b0, P::wyp1);
            P::accumulate(lo, hi, a1, b1, P::wyp2);
        }

        // Handle remaining 16-byte chunk
        if (i + 16 <= N) {
            uint64_t a = read64(p + i), b = read64(p + i + 8);
            P::accumulate(lo, hi, a, b, P::wyp3);
            i += 16;
        }

        // Final bytes (overlapping read)
        if (i < N) {
            uint64_t a = read64(p + N - 16), b = read64(p + N - 8);
            P::accumulate(lo, hi, a, b, P::wyp0);
        }

        return P::fold128(lo, hi, N);
    }
}

} // namespace state128_optimized

// ============================================================================
// OPTIMIZATION 3: AVX2 VMUM-style Vector Layer
// ============================================================================
#if defined(__AVX2__)
#include <immintrin.h>

namespace vmum_optimized {

// VMUM-style: 4-way parallel 32x32->64 multiplies using AVX2
template<size_t N>
[[gnu::always_inline]] inline uint64_t hash_bytes_vmum(const void* data) noexcept {
    const auto* p = static_cast<const uint8_t*>(data);

    auto read64 = [](const uint8_t* ptr) -> uint64_t {
        uint64_t v; memcpy(&v, ptr, 8); return v;
    };

    // For small inputs, use baseline
    if constexpr (N < 128) {
        return baseline::hash_bytes_fixed<N>(data);
    }
    else {
        // VMUM secrets
        constexpr uint64_t s0 = 0xa0761d6478bd642full;
        constexpr uint64_t s1 = 0xe7037ed1a0b428dbull;
        constexpr uint64_t s2 = 0x8ebc6af09c88c6e3ull;
        constexpr uint64_t s3 = 0x589965cc75374cc3ull;

        // Initialize 4 accumulators as 256-bit vector
        __m256i acc = _mm256_set_epi64x(s3, s2, s1, s0);
        __m256i secrets = _mm256_set_epi64x(s3 ^ N, s2 ^ N, s1 ^ N, s0 ^ N);

        size_t i = 0;
        // Process 32 bytes per iteration (4x 64-bit values)
        for (; i + 32 <= N; i += 32) {
            __m256i data_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + i));

            // XOR with secrets
            __m256i mixed = _mm256_xor_si256(data_vec, secrets);

            // 32x32->64 multiply (low 32 bits of each 64-bit lane)
            // _mm256_mul_epu32 multiplies lanes 0,2,4,6 (32-bit each)
            __m256i mul_lo = _mm256_mul_epu32(mixed, acc);

            // Shift right by 32 and multiply for high parts
            __m256i mixed_hi = _mm256_srli_epi64(mixed, 32);
            __m256i acc_hi = _mm256_srli_epi64(acc, 32);
            __m256i mul_hi = _mm256_mul_epu32(mixed_hi, acc_hi);

            // Combine: acc = mul_lo ^ mul_hi
            acc = _mm256_xor_si256(mul_lo, mul_hi);
        }

        // Reduce 256-bit accumulator to 64-bit
        // Extract 4x 64-bit values
        alignas(32) uint64_t vals[4];
        _mm256_store_si256(reinterpret_cast<__m256i*>(vals), acc);

        // Fold using standard wymix
        auto wymix = [](uint64_t a, uint64_t b) -> uint64_t {
            __uint128_t r = static_cast<__uint128_t>(a) * b;
            return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
        };

        uint64_t seed = wymix(vals[0] ^ s0, vals[1] ^ s1);
        seed = wymix(seed ^ vals[2], vals[3] ^ s2);

        // Handle tail with scalar code
        for (; i + 16 <= N; i += 16) {
            seed = wymix(read64(p + i) ^ s1, read64(p + i + 8) ^ seed);
        }

        // Final bytes
        uint64_t a = read64(p + N - 16), b = read64(p + N - 8);
        __uint128_t r = static_cast<__uint128_t>(a ^ s0 ^ N) * (b ^ s1 ^ seed);
        return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
    }
}

} // namespace vmum_optimized
#endif

// ============================================================================
// Benchmark infrastructure
// ============================================================================

inline void do_not_optimize(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

template<typename F>
double bench(F&& f, size_t iters) {
    uint64_t sink = 0;
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

// Avalanche quality test
template<typename HashFn>
double test_avalanche(HashFn hash_fn, const void* data, size_t data_size) {
    std::vector<uint8_t> buf(data_size);
    memcpy(buf.data(), data, data_size);

    uint64_t base = hash_fn(buf.data());
    double total = 0;
    size_t bits_to_test = std::min(data_size * 8, size_t(64));

    for (size_t bit = 0; bit < bits_to_test; ++bit) {
        buf[bit / 8] ^= (1 << (bit % 8));
        uint64_t flipped = hash_fn(buf.data());
        total += static_cast<double>(__builtin_popcountll(base ^ flipped)) / 64.0;
        buf[bit / 8] ^= (1 << (bit % 8));
    }
    return total / bits_to_test;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MIRROR_HASH OPTIMIZATION BENCHMARK                                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝\n\n";

    // Check CPU features
    std::cout << "CPU Features:\n";
#if defined(__AES__)
    std::cout << "  [x] AES-NI available\n";
#else
    std::cout << "  [ ] AES-NI NOT available\n";
#endif
#if defined(__AVX2__)
    std::cout << "  [x] AVX2 available\n";
#else
    std::cout << "  [ ] AVX2 NOT available\n";
#endif
    std::cout << "\n";

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

    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  PERFORMANCE BENCHMARK (nanoseconds per hash, lower = better)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(12) << "Size"
              << std::setw(12) << "wyhash"
              << std::setw(12) << "baseline"
#if defined(__AES__)
              << std::setw(12) << "AES-NI"
#endif
              << std::setw(12) << "128-state"
#if defined(__AVX2__)
              << std::setw(12) << "VMUM"
#endif
              << "\n";
    std::cout << std::string(80, '-') << "\n";

    auto run_benchmark = [&]<size_t SIZE>(const char* label, std::array<uint8_t, SIZE>& data) {
        double t_wyhash = bench([&]{ return wyhash_official(data.data(), SIZE); }, ITERS);
        double t_baseline = bench([&]{ return baseline::hash_bytes_fixed<SIZE>(data.data()); }, ITERS);
#if defined(__AES__)
        double t_aes = bench([&]{ return aes_optimized::hash_bytes_aes<SIZE>(data.data()); }, ITERS);
#endif
        double t_128state = bench([&]{ return state128_optimized::hash_bytes_128state<SIZE>(data.data()); }, ITERS);
#if defined(__AVX2__)
        double t_vmum = bench([&]{ return vmum_optimized::hash_bytes_vmum<SIZE>(data.data()); }, ITERS);
#endif

        std::cout << std::setw(12) << label
                  << std::setw(12) << t_wyhash
                  << std::setw(12) << t_baseline
#if defined(__AES__)
                  << std::setw(12) << t_aes
#endif
                  << std::setw(12) << t_128state
#if defined(__AVX2__)
                  << std::setw(12) << t_vmum
#endif
                  << "\n";
    };

    run_benchmark("8 bytes", data8);
    run_benchmark("16 bytes", data16);
    run_benchmark("32 bytes", data32);
    run_benchmark("64 bytes", data64);
    run_benchmark("96 bytes", data96);
    run_benchmark("128 bytes", data128);
    run_benchmark("256 bytes", data256);
    run_benchmark("512 bytes", data512);
    run_benchmark("1024 bytes", data1024);

    // Quality verification
    std::cout << "\n═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  HASH QUALITY - AVALANCHE TEST (ideal = 0.5000)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << std::setprecision(4);
    std::cout << std::setw(12) << "Size"
              << std::setw(12) << "wyhash"
              << std::setw(12) << "baseline"
#if defined(__AES__)
              << std::setw(12) << "AES-NI"
#endif
              << std::setw(12) << "128-state"
#if defined(__AVX2__)
              << std::setw(12) << "VMUM"
#endif
              << "\n";
    std::cout << std::string(80, '-') << "\n";

    auto run_quality = [&]<size_t SIZE>(const char* label, std::array<uint8_t, SIZE>& data) {
        auto wyhash_fn = [](const void* p) { return wyhash_official(p, SIZE); };
        auto baseline_fn = [](const void* p) { return baseline::hash_bytes_fixed<SIZE>(p); };
#if defined(__AES__)
        auto aes_fn = [](const void* p) { return aes_optimized::hash_bytes_aes<SIZE>(p); };
#endif
        auto state128_fn = [](const void* p) { return state128_optimized::hash_bytes_128state<SIZE>(p); };
#if defined(__AVX2__)
        auto vmum_fn = [](const void* p) { return vmum_optimized::hash_bytes_vmum<SIZE>(p); };
#endif

        std::cout << std::setw(12) << label
                  << std::setw(12) << test_avalanche(wyhash_fn, data.data(), SIZE)
                  << std::setw(12) << test_avalanche(baseline_fn, data.data(), SIZE)
#if defined(__AES__)
                  << std::setw(12) << test_avalanche(aes_fn, data.data(), SIZE)
#endif
                  << std::setw(12) << test_avalanche(state128_fn, data.data(), SIZE)
#if defined(__AVX2__)
                  << std::setw(12) << test_avalanche(vmum_fn, data.data(), SIZE)
#endif
                  << "\n";
    };

    run_quality("8 bytes", data8);
    run_quality("32 bytes", data32);
    run_quality("64 bytes", data64);
    run_quality("128 bytes", data128);
    run_quality("256 bytes", data256);
    run_quality("512 bytes", data512);

    // Additional collision test
    std::cout << "\n═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  COLLISION TEST (hashing 100,000 sequential values)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    auto collision_test = [](auto hash_fn, const char* name) {
        std::vector<uint64_t> hashes;
        hashes.reserve(100000);
        for (uint64_t i = 0; i < 100000; ++i) {
            alignas(8) uint64_t val = i;
            hashes.push_back(hash_fn(&val));
        }
        std::sort(hashes.begin(), hashes.end());
        size_t collisions = 0;
        for (size_t i = 1; i < hashes.size(); ++i) {
            if (hashes[i] == hashes[i-1]) ++collisions;
        }
        std::cout << "  " << std::setw(12) << name << ": " << collisions << " collisions"
                  << (collisions == 0 ? " (PASS)" : " (WARN)") << "\n";
    };

    collision_test([](const void* p) { return baseline::hash_bytes_fixed<8>(p); }, "baseline");
#if defined(__AES__)
    collision_test([](const void* p) { return aes_optimized::hash_bytes_aes<8>(p); }, "AES-NI");
#endif
    collision_test([](const void* p) { return state128_optimized::hash_bytes_128state<8>(p); }, "128-state");

    // Bit independence criterion
    std::cout << "\n═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  BIT INDEPENDENCE CRITERION (correlation between output bits)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    auto bic_test = [&rng](auto hash_fn, const char* name, size_t size) {
        // Generate random inputs and measure bit correlations
        std::vector<uint8_t> data(size);
        int corr_count[64][64] = {{0}};
        constexpr int SAMPLES = 10000;

        for (int s = 0; s < SAMPLES; ++s) {
            for (auto& b : data) b = rng() & 0xFF;
            uint64_t h = hash_fn(data.data());
            for (int i = 0; i < 64; ++i) {
                for (int j = i + 1; j < 64; ++j) {
                    int bi = (h >> i) & 1;
                    int bj = (h >> j) & 1;
                    if (bi == bj) corr_count[i][j]++;
                }
            }
        }

        // Calculate max deviation from 0.5
        double max_dev = 0.0;
        for (int i = 0; i < 64; ++i) {
            for (int j = i + 1; j < 64; ++j) {
                double ratio = static_cast<double>(corr_count[i][j]) / SAMPLES;
                double dev = std::abs(ratio - 0.5);
                if (dev > max_dev) max_dev = dev;
            }
        }

        bool pass = max_dev < 0.05;  // 5% threshold
        std::cout << "  " << std::setw(12) << name << " (" << std::setw(4) << size << "B): "
                  << "max_correlation=" << std::setprecision(4) << max_dev
                  << (pass ? " (PASS)" : " (FAIL)") << "\n";
    };

    bic_test([](const void* p) { return baseline::hash_bytes_fixed<32>(p); }, "baseline", 32);
    bic_test([](const void* p) { return baseline::hash_bytes_fixed<128>(p); }, "baseline", 128);
#if defined(__AES__)
    bic_test([](const void* p) { return aes_optimized::hash_bytes_aes<32>(p); }, "AES-NI", 32);
    bic_test([](const void* p) { return aes_optimized::hash_bytes_aes<128>(p); }, "AES-NI", 128);
#endif
    bic_test([](const void* p) { return state128_optimized::hash_bytes_128state<128>(p); }, "128-state", 128);

    std::cout << "\n═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  SUMMARY\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << "Optimizations tested:\n";
    std::cout << "  1. AES-NI: Hardware AES instructions for mixing (best for large inputs)\n";
    std::cout << "  2. 128-state: Delayed folding with 128-bit accumulation\n";
    std::cout << "  3. VMUM: AVX2 vector 32x32->64 multiplies (needs more work)\n\n";

    std::cout << "Quality thresholds:\n";
    std::cout << "  - Avalanche ratio: 0.48-0.52 (ideal = 0.50)\n";
    std::cout << "  - Collisions: 0 for 100K sequential values\n";
    std::cout << "  - Bit independence: max correlation < 0.05\n";

    return 0;
}
