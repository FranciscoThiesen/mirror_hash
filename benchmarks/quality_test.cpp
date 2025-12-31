// Quick hash quality verification for the optimized implementation
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <set>
#include <vector>
#include <random>

#include "../include/mirror_hash_unified.hpp"

// Simple avalanche test: flip each input bit, check output changes
bool test_avalanche(size_t input_len) {
    std::vector<uint8_t> input(input_len);
    std::mt19937_64 rng(42);
    for (auto& b : input) b = rng() & 0xFF;

    uint64_t original = mirror_hash::hash(input.data(), input_len, 0);

    int total_bits_changed = 0;
    int total_tests = 0;

    for (size_t byte = 0; byte < input_len; byte++) {
        for (int bit = 0; bit < 8; bit++) {
            input[byte] ^= (1 << bit);
            uint64_t modified = mirror_hash::hash(input.data(), input_len, 0);
            input[byte] ^= (1 << bit);

            uint64_t diff = original ^ modified;
            int bits_changed = __builtin_popcountll(diff);
            total_bits_changed += bits_changed;
            total_tests++;
        }
    }

    double avg_bits = static_cast<double>(total_bits_changed) / total_tests;
    // Good avalanche: ~32 bits change on average (half of 64)
    bool pass = avg_bits >= 28 && avg_bits <= 36;
    printf("  len=%zu: avg bits changed = %.1f/64 %s\n", input_len, avg_bits, pass ? "PASS" : "FAIL");
    return pass;
}

// Collision test: hash many similar inputs, check for collisions
bool test_collisions(size_t input_len, size_t count) {
    std::set<uint64_t> hashes;
    std::vector<uint8_t> input(input_len);

    for (size_t i = 0; i < count; i++) {
        // Use counter as input
        for (size_t j = 0; j < input_len && j < 8; j++) {
            input[j] = (i >> (j * 8)) & 0xFF;
        }
        uint64_t h = mirror_hash::hash(input.data(), input_len, 0);
        hashes.insert(h);
    }

    bool pass = hashes.size() == count;
    printf("  len=%zu: %zu unique hashes out of %zu %s\n",
           input_len, hashes.size(), count, pass ? "PASS" : "FAIL");
    return pass;
}

// Seed test: different seeds should produce different hashes
bool test_seed_sensitivity(size_t input_len) {
    std::vector<uint8_t> input(input_len);
    std::mt19937_64 rng(42);
    for (auto& b : input) b = rng() & 0xFF;

    std::set<uint64_t> hashes;
    for (uint64_t seed = 0; seed < 1000; seed++) {
        uint64_t h = mirror_hash::hash(input.data(), input_len, seed);
        hashes.insert(h);
    }

    bool pass = hashes.size() == 1000;
    printf("  len=%zu: %zu unique hashes for 1000 seeds %s\n",
           input_len, hashes.size(), pass ? "PASS" : "FAIL");
    return pass;
}

int main() {
    printf("=== Hash Quality Verification ===\n\n");

    bool all_pass = true;

    // Test key sizes in the optimized range
    std::vector<size_t> sizes = {33, 48, 64, 80, 96, 112, 128};

    printf("Avalanche Test:\n");
    for (size_t sz : sizes) {
        if (!test_avalanche(sz)) all_pass = false;
    }

    printf("\nCollision Test (10000 inputs):\n");
    for (size_t sz : sizes) {
        if (!test_collisions(sz, 10000)) all_pass = false;
    }

    printf("\nSeed Sensitivity Test:\n");
    for (size_t sz : sizes) {
        if (!test_seed_sensitivity(sz)) all_pass = false;
    }

    printf("\n=== Overall: %s ===\n", all_pass ? "ALL TESTS PASSED" : "SOME TESTS FAILED");

    return all_pass ? 0 : 1;
}
