// Core SMHasher-style tests for mirror_hash
// Tests: Sanity, Avalanche, Bit Independence, Differential, Sparse Keys
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <set>
#include <random>
#include <cmath>
#include <algorithm>

#include "../include/mirror_hash_unified.hpp"

using HashFn = uint64_t(*)(const void*, size_t, uint64_t);

uint64_t mirror_hash_fn(const void* key, size_t len, uint64_t seed) {
    return mirror_hash::hash(key, len, seed);
}

// ============================================================================
// Test 1: Sanity Check
// ============================================================================
bool test_sanity() {
    printf("Sanity Check:\n");

    // Same input should produce same output
    uint8_t data[64];
    for (int i = 0; i < 64; i++) data[i] = i;

    uint64_t h1 = mirror_hash::hash(data, 64, 0);
    uint64_t h2 = mirror_hash::hash(data, 64, 0);

    if (h1 != h2) {
        printf("  FAIL: Non-deterministic hash\n");
        return false;
    }

    // Different seeds should produce different hashes
    uint64_t h3 = mirror_hash::hash(data, 64, 1);
    if (h1 == h3) {
        printf("  FAIL: Seeds don't affect output\n");
        return false;
    }

    // Zero-length should work
    uint64_t h4 = mirror_hash::hash(data, 0, 0);
    (void)h4; // Just verify it doesn't crash

    printf("  PASS\n");
    return true;
}

// ============================================================================
// Test 2: Avalanche Test (Strict Avalanche Criterion)
// For each input bit flip, ~50% of output bits should flip
// ============================================================================
bool test_avalanche() {
    printf("Avalanche Test:\n");

    std::mt19937_64 rng(42);
    const int REPS = 1000;
    bool all_pass = true;

    for (size_t len : {33, 48, 64, 80, 96, 112, 128}) {
        std::vector<uint8_t> input(len);
        double total_bias = 0;
        int tests = 0;

        for (int rep = 0; rep < REPS; rep++) {
            for (auto& b : input) b = rng() & 0xFF;
            uint64_t original = mirror_hash::hash(input.data(), len, 0);

            // Flip each input bit
            for (size_t byte = 0; byte < len; byte++) {
                for (int bit = 0; bit < 8; bit++) {
                    input[byte] ^= (1 << bit);
                    uint64_t modified = mirror_hash::hash(input.data(), len, 0);
                    input[byte] ^= (1 << bit);

                    uint64_t diff = original ^ modified;
                    int bits_changed = __builtin_popcountll(diff);

                    // Bias from ideal 32 bits
                    total_bias += std::abs(bits_changed - 32.0);
                    tests++;
                }
            }
        }

        double avg_bias = total_bias / tests;
        // Good avalanche: average bias should be < 5 bits from ideal
        bool pass = avg_bias < 5.0;
        printf("  len=%zu: avg bias from 32 = %.2f %s\n", len, avg_bias, pass ? "PASS" : "FAIL");
        if (!pass) all_pass = false;
    }

    return all_pass;
}

// ============================================================================
// Test 3: Bit Independence Criterion
// Changes in different output bits should be independent
// ============================================================================
bool test_bit_independence() {
    printf("Bit Independence Test:\n");

    std::mt19937_64 rng(123);
    const int REPS = 5000;

    // Count co-occurrence of bit flips for pairs of output bits
    std::vector<std::vector<int>> coflip(64, std::vector<int>(64, 0));
    std::vector<int> flip_count(64, 0);

    for (size_t len : {64, 128}) {
        std::vector<uint8_t> input(len);

        for (int rep = 0; rep < REPS; rep++) {
            for (auto& b : input) b = rng() & 0xFF;
            uint64_t original = mirror_hash::hash(input.data(), len, 0);

            // Flip a random input bit
            size_t byte = rng() % len;
            int bit = rng() % 8;
            input[byte] ^= (1 << bit);
            uint64_t modified = mirror_hash::hash(input.data(), len, 0);
            input[byte] ^= (1 << bit);

            uint64_t diff = original ^ modified;

            // Count which output bits flipped
            for (int i = 0; i < 64; i++) {
                if (diff & (1ULL << i)) {
                    flip_count[i]++;
                    for (int j = i + 1; j < 64; j++) {
                        if (diff & (1ULL << j)) {
                            coflip[i][j]++;
                        }
                    }
                }
            }
        }
    }

    // Check independence: P(A and B) should be close to P(A) * P(B)
    double max_correlation = 0;
    for (int i = 0; i < 64; i++) {
        for (int j = i + 1; j < 64; j++) {
            double p_i = flip_count[i] / (double)(REPS * 2);
            double p_j = flip_count[j] / (double)(REPS * 2);
            double p_ij = coflip[i][j] / (double)(REPS * 2);

            double expected = p_i * p_j;
            if (expected > 0.01) {  // Only check significant probabilities
                double correlation = std::abs(p_ij - expected) / expected;
                max_correlation = std::max(max_correlation, correlation);
            }
        }
    }

    bool pass = max_correlation < 0.3;  // Allow 30% deviation from independence
    printf("  Max correlation deviation: %.1f%% %s\n", max_correlation * 100, pass ? "PASS" : "FAIL");
    return pass;
}

// ============================================================================
// Test 4: Differential Test
// Similar inputs should produce very different hashes
// ============================================================================
bool test_differential() {
    printf("Differential Test:\n");

    std::mt19937_64 rng(456);
    const int REPS = 10000;

    bool all_pass = true;

    for (size_t len : {33, 64, 128}) {
        std::vector<uint8_t> input1(len);
        std::vector<uint8_t> input2(len);

        int min_hamming = 64;
        double total_hamming = 0;

        for (int rep = 0; rep < REPS; rep++) {
            for (auto& b : input1) b = rng() & 0xFF;
            memcpy(input2.data(), input1.data(), len);

            // Change just 1 byte
            input2[rng() % len] ^= (1 << (rng() % 8));

            uint64_t h1 = mirror_hash::hash(input1.data(), len, 0);
            uint64_t h2 = mirror_hash::hash(input2.data(), len, 0);

            int hamming = __builtin_popcountll(h1 ^ h2);
            min_hamming = std::min(min_hamming, hamming);
            total_hamming += hamming;
        }

        double avg_hamming = total_hamming / REPS;
        // Good differential: average should be ~32, minimum should be > 10
        bool pass = avg_hamming > 28 && min_hamming > 5;
        printf("  len=%zu: avg=%.1f, min=%d %s\n", len, avg_hamming, min_hamming, pass ? "PASS" : "FAIL");
        if (!pass) all_pass = false;
    }

    return all_pass;
}

// ============================================================================
// Test 5: Sparse Key Test
// Test with keys that have very few bits set
// ============================================================================
bool test_sparse() {
    printf("Sparse Key Test:\n");

    std::set<uint64_t> hashes;
    const size_t len = 64;
    std::vector<uint8_t> key(len, 0);

    // Test single-bit keys
    for (size_t i = 0; i < len * 8; i++) {
        memset(key.data(), 0, len);
        key[i / 8] = 1 << (i % 8);
        hashes.insert(mirror_hash::hash(key.data(), len, 0));
    }

    // Test two-bit keys
    for (size_t i = 0; i < len * 8; i++) {
        for (size_t j = i + 1; j < len * 8 && hashes.size() < 100000; j++) {
            memset(key.data(), 0, len);
            key[i / 8] |= 1 << (i % 8);
            key[j / 8] |= 1 << (j % 8);
            hashes.insert(mirror_hash::hash(key.data(), len, 0));
        }
    }

    size_t expected = len * 8 + (len * 8) * (len * 8 - 1) / 2;
    expected = std::min(expected, (size_t)100000);

    bool pass = hashes.size() == expected;
    printf("  %zu unique hashes from %zu sparse keys %s\n",
           hashes.size(), expected, pass ? "PASS" : "FAIL");
    return pass;
}

// ============================================================================
// Test 6: Permutation Test
// Keys that are permutations of each other should hash differently
// ============================================================================
bool test_permutation() {
    printf("Permutation Test:\n");

    std::set<uint64_t> hashes;
    std::vector<uint8_t> key = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
                                32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};

    // Generate many permutations
    std::mt19937_64 rng(789);
    for (int i = 0; i < 10000; i++) {
        std::shuffle(key.begin(), key.end(), rng);
        hashes.insert(mirror_hash::hash(key.data(), key.size(), 0));
    }

    bool pass = hashes.size() == 10000;
    printf("  %zu unique hashes from 10000 permutations %s\n",
           hashes.size(), pass ? "PASS" : "FAIL");
    return pass;
}

// ============================================================================
// Test 7: Cyclic Test
// Test cyclic rotations of the same data
// ============================================================================
bool test_cyclic() {
    printf("Cyclic Test:\n");

    std::set<uint64_t> hashes;
    std::vector<uint8_t> key(64);
    std::mt19937_64 rng(101);
    for (auto& b : key) b = rng() & 0xFF;

    // All cyclic rotations
    for (size_t i = 0; i < 64; i++) {
        std::vector<uint8_t> rotated(64);
        for (size_t j = 0; j < 64; j++) {
            rotated[j] = key[(j + i) % 64];
        }
        hashes.insert(mirror_hash::hash(rotated.data(), 64, 0));
    }

    bool pass = hashes.size() == 64;
    printf("  %zu unique hashes from 64 cyclic rotations %s\n",
           hashes.size(), pass ? "PASS" : "FAIL");
    return pass;
}

// ============================================================================
// Main
// ============================================================================
int main() {
    printf("=== SMHasher Core Tests for mirror_hash_unified v2.1 ===\n\n");

    bool all_pass = true;

    all_pass &= test_sanity();
    all_pass &= test_avalanche();
    all_pass &= test_bit_independence();
    all_pass &= test_differential();
    all_pass &= test_sparse();
    all_pass &= test_permutation();
    all_pass &= test_cyclic();

    printf("\n=== OVERALL: %s ===\n", all_pass ? "ALL TESTS PASSED" : "SOME TESTS FAILED");

    return all_pass ? 0 : 1;
}
