// Benchmark to test if overlapping reads actually provide a measurable benefit
// Compare: overlapping read vs memcpy-to-buffer for handling 1-15 byte remainders

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <arm_neon.h>

using Clock = std::chrono::high_resolution_clock;
volatile uint64_t sink = 0;

// AES keys (same as in implementation)
alignas(16) static constexpr uint8_t AES_KEY1[16] = {
    0x2d, 0x35, 0x8d, 0xcc, 0xaa, 0x6c, 0x78, 0xa5,
    0x8b, 0xb8, 0x4b, 0x93, 0x96, 0x2e, 0xac, 0xc9
};
alignas(16) static constexpr uint8_t AES_KEY2[16] = {
    0x4b, 0x33, 0xa6, 0x2e, 0xd4, 0x33, 0xd4, 0xa3,
    0x4d, 0x5a, 0x2d, 0xa5, 0x1d, 0xe1, 0xaa, 0x47
};

[[gnu::always_inline]] inline
uint8x16_t aes_mix(uint8x16_t data, uint8x16_t key) noexcept {
    return vaesmcq_u8(vaeseq_u8(data, key));
}

[[gnu::always_inline]] inline
uint64_t finalize_aes(uint8x16_t state) noexcept {
    uint64x2_t v64 = vreinterpretq_u64_u8(state);
    return vgetq_lane_u64(v64, 0) ^ vgetq_lane_u64(v64, 1);
}

// VERSION 1: With overlapping read (current implementation)
[[gnu::noinline]]
uint64_t hash_with_overlapping_read(const void* key, size_t len, uint64_t seed) noexcept {
    const uint8_t* p = static_cast<const uint8_t*>(key);
    const uint8_t* const end = p + len;

    uint8x16_t k1 = vld1q_u8(AES_KEY1);
    uint8x16_t k2 = vld1q_u8(AES_KEY2);
    uint64x2_t seed_vec = vdupq_n_u64(seed);
    uint8x16_t state = vreinterpretq_u8_u64(seed_vec);

    while (len >= 32) {
        uint8x16_t d0 = vld1q_u8(p);
        uint8x16_t d1 = vld1q_u8(p + 16);
        state = veorq_u8(state, d0);
        state = aes_mix(state, k1);
        state = veorq_u8(state, d1);
        state = aes_mix(state, k2);
        p += 32;
        len -= 32;
    }

    if (len >= 16) {
        uint8x16_t data = vld1q_u8(p);
        state = veorq_u8(state, data);
        state = aes_mix(state, k1);
        p += 16;
        len -= 16;
    }

    // OVERLAPPING READ: read last 16 bytes
    if (len > 0) {
        uint8x16_t data = vld1q_u8(end - 16);
        uint8x16_t len_vec = vdupq_n_u8(static_cast<uint8_t>(len));
        data = veorq_u8(data, len_vec);
        state = veorq_u8(state, data);
        state = aes_mix(state, k2);
    }

    state = aes_mix(state, k1);
    state = aes_mix(state, k2);
    return finalize_aes(state);
}

// VERSION 2: With memcpy to buffer (traditional approach)
[[gnu::noinline]]
uint64_t hash_with_memcpy(const void* key, size_t len, uint64_t seed) noexcept {
    const uint8_t* p = static_cast<const uint8_t*>(key);

    uint8x16_t k1 = vld1q_u8(AES_KEY1);
    uint8x16_t k2 = vld1q_u8(AES_KEY2);
    uint64x2_t seed_vec = vdupq_n_u64(seed);
    uint8x16_t state = vreinterpretq_u8_u64(seed_vec);

    while (len >= 32) {
        uint8x16_t d0 = vld1q_u8(p);
        uint8x16_t d1 = vld1q_u8(p + 16);
        state = veorq_u8(state, d0);
        state = aes_mix(state, k1);
        state = veorq_u8(state, d1);
        state = aes_mix(state, k2);
        p += 32;
        len -= 32;
    }

    if (len >= 16) {
        uint8x16_t data = vld1q_u8(p);
        state = veorq_u8(state, data);
        state = aes_mix(state, k1);
        p += 16;
        len -= 16;
    }

    // MEMCPY APPROACH: copy to zeroed buffer
    if (len > 0) {
        alignas(16) uint8_t buf[16] = {0};
        std::memcpy(buf, p, len);
        buf[15] = static_cast<uint8_t>(len);
        uint8x16_t data = vld1q_u8(buf);
        state = veorq_u8(state, data);
        state = aes_mix(state, k2);
    }

    state = aes_mix(state, k1);
    state = aes_mix(state, k2);
    return finalize_aes(state);
}

template<typename HashFn>
double benchmark(HashFn fn, const void* data, size_t len, size_t iterations) {
    // Warmup
    uint64_t warmup = 0;
    for (size_t i = 0; i < 10000; i++) warmup += fn(data, len, i);
    sink = warmup;

    std::vector<double> times;
    for (int run = 0; run < 15; run++) {
        auto start = Clock::now();
        uint64_t sum = 0;
        for (size_t i = 0; i < iterations; i++) {
            sum += fn(data, len, i);
        }
        auto end = Clock::now();
        sink = sum;
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        times.push_back(static_cast<double>(ns) / iterations);
    }
    std::sort(times.begin(), times.end());
    return times[7]; // median
}

int main() {
    printf("=== Overlapping Read vs Memcpy Benchmark ===\n\n");
    printf("Testing if overlapping reads provide measurable benefit.\n");
    printf("Only sizes with 1-15 byte remainders use the partial block handling.\n\n");

    std::mt19937_64 rng(12345);

    // Test sizes that have remainders (where the optimization matters)
    std::vector<size_t> sizes_with_remainder = {33, 35, 40, 47, 49, 55, 63, 65, 70, 79, 81, 90, 95, 97, 100, 110, 113, 120, 127};

    // Test sizes without remainder (control group - should be identical)
    std::vector<size_t> sizes_no_remainder = {48, 64, 80, 96, 112, 128};

    printf("=== Sizes WITH remainder (optimization should matter) ===\n");
    printf("| Size | Overlapping | Memcpy | Diff | Winner |\n");
    printf("|------|-------------|--------|------|--------|\n");

    double total_overlap_time = 0, total_memcpy_time = 0;
    int overlap_wins = 0, memcpy_wins = 0;

    for (size_t sz : sizes_with_remainder) {
        std::vector<uint8_t> data(sz);
        for (auto& b : data) b = rng() & 0xFF;

        size_t iters = 5000000;
        double t_overlap = benchmark(hash_with_overlapping_read, data.data(), sz, iters);
        double t_memcpy = benchmark(hash_with_memcpy, data.data(), sz, iters);

        double diff_pct = (t_memcpy / t_overlap - 1.0) * 100;
        const char* winner = diff_pct > 1.0 ? "overlap" : (diff_pct < -1.0 ? "memcpy" : "~even");

        if (diff_pct > 1.0) overlap_wins++;
        else if (diff_pct < -1.0) memcpy_wins++;

        total_overlap_time += t_overlap;
        total_memcpy_time += t_memcpy;

        printf("| %3zuB | %6.2f ns | %6.2f ns | %+5.1f%% | %s |\n",
               sz, t_overlap, t_memcpy, diff_pct, winner);
    }

    printf("\n=== Sizes WITHOUT remainder (control - should be identical) ===\n");
    printf("| Size | Overlapping | Memcpy | Diff |\n");
    printf("|------|-------------|--------|------|\n");

    for (size_t sz : sizes_no_remainder) {
        std::vector<uint8_t> data(sz);
        for (auto& b : data) b = rng() & 0xFF;

        size_t iters = 5000000;
        double t_overlap = benchmark(hash_with_overlapping_read, data.data(), sz, iters);
        double t_memcpy = benchmark(hash_with_memcpy, data.data(), sz, iters);

        double diff_pct = (t_memcpy / t_overlap - 1.0) * 100;
        printf("| %3zuB | %6.2f ns | %6.2f ns | %+5.1f%% |\n",
               sz, t_overlap, t_memcpy, diff_pct);
    }

    printf("\n=== Summary ===\n");
    printf("Sizes with remainder:\n");
    printf("  Overlapping read wins: %d/%zu\n", overlap_wins, sizes_with_remainder.size());
    printf("  Memcpy wins: %d/%zu\n", memcpy_wins, sizes_with_remainder.size());
    printf("  Average time (overlapping): %.2f ns\n", total_overlap_time / sizes_with_remainder.size());
    printf("  Average time (memcpy): %.2f ns\n", total_memcpy_time / sizes_with_remainder.size());
    double avg_benefit = (total_memcpy_time / total_overlap_time - 1.0) * 100;
    printf("  Average benefit of overlapping read: %+.1f%%\n", avg_benefit);

    if (avg_benefit > 2.0) {
        printf("\nConclusion: Overlapping read provides MEASURABLE benefit (%.1f%% faster)\n", avg_benefit);
    } else if (avg_benefit < -2.0) {
        printf("\nConclusion: Memcpy is actually FASTER (%.1f%% faster)\n", -avg_benefit);
    } else {
        printf("\nConclusion: NO SIGNIFICANT DIFFERENCE between the two approaches\n");
    }

    return 0;
}
