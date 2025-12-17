#pragma once

// ============================================================================
// Hash Quality Analysis Framework
// ============================================================================
//
// This framework implements rigorous statistical tests for hash function
// quality, based on established cryptographic and hash function literature:
//
// References:
// - Webster & Tavares (1985): "On the Design of S-Boxes"
//   Introduced Strict Avalanche Criterion (SAC) and Bit Independence Criterion
// - Knuth, TAOCP Vol 3: Chi-squared tests for distribution uniformity
// - SMHasher by Austin Appleby: Industry-standard hash test suite
// - Estébanez et al. (2014): "Automatic Generation of Hash Functions"
//
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <cmath>
#include <random>
#include <algorithm>
#include <numeric>
#include <functional>
#include <string>
#include <unordered_set>
#include <iostream>
#include <iomanip>
#include <bitset>

namespace hash_quality {

// ============================================================================
// Core Utilities
// ============================================================================

constexpr int HASH_BITS = 64;

inline int popcount(uint64_t x) {
    return __builtin_popcountll(x);
}

inline uint64_t rotl64(uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

// ============================================================================
// Avalanche Analysis
// ============================================================================
//
// The Strict Avalanche Criterion (SAC) states that when a single input bit
// is flipped, each output bit should change with probability 0.5.
//
// For a 64-bit hash, flipping any input bit should change ~32 output bits.
// We measure the "avalanche ratio" as (bits changed) / (total bits).
// Perfect avalanche ratio = 0.5
//
// Metrics:
// - Mean avalanche ratio: average across all input bits and samples
// - Avalanche bias: |mean - 0.5|, lower is better (0 = perfect)
// - Per-bit avalanche: how well each output bit satisfies SAC
//
// ============================================================================

struct AvalancheResult {
    double mean_avalanche_ratio;      // Average bits changed / total bits
    double avalanche_bias;            // |mean - 0.5|, 0 = perfect
    double min_avalanche_ratio;       // Worst case (too few changes)
    double max_avalanche_ratio;       // Worst case (too many changes)
    double std_deviation;             // Consistency of avalanche
    std::array<double, HASH_BITS> per_bit_flip_probability;  // SAC per output bit
    double sac_bias;                  // Mean |P(bit flip) - 0.5| across all output bits
    bool passed;                      // Overall pass/fail

    void print() const {
        std::cout << "=== Avalanche Analysis ===\n";
        std::cout << "Mean avalanche ratio:    " << std::fixed << std::setprecision(4)
                  << mean_avalanche_ratio << " (ideal: 0.5000)\n";
        std::cout << "Avalanche bias:          " << avalanche_bias
                  << " (ideal: 0.0000)\n";
        std::cout << "Range:                   [" << min_avalanche_ratio
                  << ", " << max_avalanche_ratio << "]\n";
        std::cout << "Standard deviation:      " << std_deviation << "\n";
        std::cout << "SAC bias (per-bit):      " << sac_bias
                  << " (ideal: 0.0000)\n";
        std::cout << "Result:                  " << (passed ? "PASS" : "FAIL") << "\n";
    }
};

template<typename HashFn>
AvalancheResult analyze_avalanche(HashFn hash_fn, std::size_t num_samples = 100000) {
    std::mt19937_64 rng(42);

    // Track how often each output bit flips when any input bit flips
    std::array<uint64_t, HASH_BITS> output_bit_flip_count{};
    uint64_t total_bit_tests = 0;

    std::vector<double> avalanche_ratios;
    avalanche_ratios.reserve(num_samples * 64);  // For each input bit flip

    for (std::size_t i = 0; i < num_samples; ++i) {
        // Generate random 64-bit input
        uint64_t input = rng();
        uint64_t base_hash = hash_fn(&input, sizeof(input));

        // Flip each input bit and measure output change
        for (int bit = 0; bit < 64; ++bit) {
            uint64_t flipped_input = input ^ (1ULL << bit);
            uint64_t flipped_hash = hash_fn(&flipped_input, sizeof(flipped_input));

            uint64_t diff = base_hash ^ flipped_hash;
            int bits_changed = popcount(diff);

            avalanche_ratios.push_back(static_cast<double>(bits_changed) / HASH_BITS);

            // Track per-output-bit flip probability
            for (int out_bit = 0; out_bit < HASH_BITS; ++out_bit) {
                if (diff & (1ULL << out_bit)) {
                    output_bit_flip_count[out_bit]++;
                }
            }
            total_bit_tests++;
        }
    }

    AvalancheResult result;

    // Calculate mean
    result.mean_avalanche_ratio = std::accumulate(
        avalanche_ratios.begin(), avalanche_ratios.end(), 0.0) / avalanche_ratios.size();

    result.avalanche_bias = std::abs(result.mean_avalanche_ratio - 0.5);

    // Min/max
    auto [min_it, max_it] = std::minmax_element(
        avalanche_ratios.begin(), avalanche_ratios.end());
    result.min_avalanche_ratio = *min_it;
    result.max_avalanche_ratio = *max_it;

    // Standard deviation
    double variance = 0;
    for (double r : avalanche_ratios) {
        variance += (r - result.mean_avalanche_ratio) * (r - result.mean_avalanche_ratio);
    }
    result.std_deviation = std::sqrt(variance / avalanche_ratios.size());

    // Per-bit flip probabilities (SAC)
    result.sac_bias = 0;
    for (int bit = 0; bit < HASH_BITS; ++bit) {
        result.per_bit_flip_probability[bit] =
            static_cast<double>(output_bit_flip_count[bit]) / total_bit_tests;
        result.sac_bias += std::abs(result.per_bit_flip_probability[bit] - 0.5);
    }
    result.sac_bias /= HASH_BITS;

    // Pass criteria: bias < 0.02 (within 2% of ideal)
    result.passed = result.avalanche_bias < 0.02 && result.sac_bias < 0.02;

    return result;
}

// ============================================================================
// Bit Independence Criterion (BIC)
// ============================================================================
//
// Tests whether changes in output bits are independent of each other.
// When an input bit is flipped, the correlation between any two output
// bits changing should be close to 0 (independent).
//
// We compute the correlation matrix between output bit changes.
// Perfect BIC: all off-diagonal correlations ≈ 0
//
// ============================================================================

struct BICResult {
    double mean_correlation;          // Average |correlation| between output bits
    double max_correlation;           // Worst case correlation
    std::vector<std::pair<int, int>> highly_correlated_pairs;  // Pairs with |r| > 0.1
    bool passed;

    void print() const {
        std::cout << "=== Bit Independence Criterion ===\n";
        std::cout << "Mean |correlation|:      " << std::fixed << std::setprecision(4)
                  << mean_correlation << " (ideal: 0.0000)\n";
        std::cout << "Max |correlation|:       " << max_correlation << "\n";
        std::cout << "Correlated pairs (>0.1): " << highly_correlated_pairs.size() << "\n";
        std::cout << "Result:                  " << (passed ? "PASS" : "FAIL") << "\n";
    }
};

template<typename HashFn>
BICResult analyze_bit_independence(HashFn hash_fn, std::size_t num_samples = 50000) {
    std::mt19937_64 rng(42);

    // Track co-occurrence of bit flips: count[i][j] = times bits i and j both flipped
    std::array<std::array<uint64_t, HASH_BITS>, HASH_BITS> coflip_count{};
    std::array<uint64_t, HASH_BITS> flip_count{};
    uint64_t total_tests = 0;

    for (std::size_t i = 0; i < num_samples; ++i) {
        uint64_t input = rng();
        uint64_t base_hash = hash_fn(&input, sizeof(input));

        for (int bit = 0; bit < 64; ++bit) {
            uint64_t flipped_input = input ^ (1ULL << bit);
            uint64_t flipped_hash = hash_fn(&flipped_input, sizeof(flipped_input));
            uint64_t diff = base_hash ^ flipped_hash;

            // Record which output bits flipped
            for (int out_i = 0; out_i < HASH_BITS; ++out_i) {
                bool i_flipped = (diff >> out_i) & 1;
                if (i_flipped) {
                    flip_count[out_i]++;
                    for (int out_j = out_i + 1; out_j < HASH_BITS; ++out_j) {
                        bool j_flipped = (diff >> out_j) & 1;
                        if (j_flipped) {
                            coflip_count[out_i][out_j]++;
                        }
                    }
                }
            }
            total_tests++;
        }
    }

    BICResult result;
    result.mean_correlation = 0;
    result.max_correlation = 0;
    int pair_count = 0;

    // Compute correlation: r = (P(A∧B) - P(A)P(B)) / sqrt(P(A)(1-P(A))P(B)(1-P(B)))
    for (int i = 0; i < HASH_BITS; ++i) {
        for (int j = i + 1; j < HASH_BITS; ++j) {
            double p_i = static_cast<double>(flip_count[i]) / total_tests;
            double p_j = static_cast<double>(flip_count[j]) / total_tests;
            double p_ij = static_cast<double>(coflip_count[i][j]) / total_tests;

            // Avoid division by zero
            double denom = std::sqrt(p_i * (1 - p_i) * p_j * (1 - p_j));
            if (denom < 1e-10) continue;

            double correlation = (p_ij - p_i * p_j) / denom;
            double abs_corr = std::abs(correlation);

            result.mean_correlation += abs_corr;
            result.max_correlation = std::max(result.max_correlation, abs_corr);
            pair_count++;

            if (abs_corr > 0.1) {
                result.highly_correlated_pairs.emplace_back(i, j);
            }
        }
    }

    result.mean_correlation /= pair_count;
    result.passed = result.max_correlation < 0.1 && result.mean_correlation < 0.02;

    return result;
}

// ============================================================================
// Chi-Squared Distribution Test
// ============================================================================
//
// Tests whether hash values are uniformly distributed across buckets.
// Based on Pearson's chi-squared test for goodness of fit.
//
// For N items in B buckets, expected count E = N/B
// Chi-squared statistic: χ² = Σ(observed - expected)² / expected
//
// For B-1 degrees of freedom, we compare against critical values:
// - α=0.01: if χ² < critical, distribution is uniform at 99% confidence
//
// ============================================================================

struct ChiSquaredResult {
    double chi_squared;               // The χ² statistic
    double degrees_of_freedom;        // B - 1
    double p_value;                   // Probability of this χ² by chance
    double expected_per_bucket;       // N / B
    double actual_variance;           // Variance of bucket counts
    double expected_variance;         // Expected variance for uniform distribution
    int empty_buckets;                // Number of empty buckets
    bool passed;

    void print() const {
        std::cout << "=== Chi-Squared Distribution Test ===\n";
        std::cout << "Chi-squared statistic:   " << std::fixed << std::setprecision(2)
                  << chi_squared << "\n";
        std::cout << "Degrees of freedom:      " << degrees_of_freedom << "\n";
        std::cout << "Expected per bucket:     " << expected_per_bucket << "\n";
        std::cout << "Empty buckets:           " << empty_buckets << "\n";
        std::cout << "Variance ratio:          " << std::setprecision(4)
                  << (actual_variance / expected_variance)
                  << " (ideal: 1.0)\n";
        std::cout << "Result:                  " << (passed ? "PASS" : "FAIL") << "\n";
    }
};

template<typename HashFn>
ChiSquaredResult analyze_distribution(HashFn hash_fn,
                                       std::size_t num_samples = 1000000,
                                       std::size_t num_buckets = 65536) {
    std::mt19937_64 rng(42);
    std::vector<uint64_t> bucket_counts(num_buckets, 0);

    for (std::size_t i = 0; i < num_samples; ++i) {
        uint64_t input = rng();
        uint64_t h = hash_fn(&input, sizeof(input));
        bucket_counts[h % num_buckets]++;
    }

    ChiSquaredResult result;
    result.degrees_of_freedom = num_buckets - 1;
    result.expected_per_bucket = static_cast<double>(num_samples) / num_buckets;

    // Calculate chi-squared
    result.chi_squared = 0;
    result.empty_buckets = 0;
    double sum = 0, sum_sq = 0;

    for (std::size_t i = 0; i < num_buckets; ++i) {
        double observed = bucket_counts[i];
        double diff = observed - result.expected_per_bucket;
        result.chi_squared += (diff * diff) / result.expected_per_bucket;

        sum += observed;
        sum_sq += observed * observed;

        if (bucket_counts[i] == 0) result.empty_buckets++;
    }

    // Variance analysis
    double mean = sum / num_buckets;
    result.actual_variance = (sum_sq / num_buckets) - (mean * mean);
    // For Poisson distribution (ideal): variance = mean
    result.expected_variance = result.expected_per_bucket;

    // Approximate p-value using normal approximation for large df
    // For large df: (χ² - df) / sqrt(2*df) ~ N(0,1)
    double z = (result.chi_squared - result.degrees_of_freedom) /
               std::sqrt(2.0 * result.degrees_of_freedom);
    // Two-tailed test: we care about both too uniform and too clustered
    result.p_value = 2.0 * (1.0 - 0.5 * std::erfc(-z / std::sqrt(2.0)));
    if (result.p_value > 1.0) result.p_value = 2.0 - result.p_value;

    // Pass if p-value is reasonable (not suspiciously uniform or clustered)
    // p < 0.001 suggests non-random behavior
    result.passed = result.p_value > 0.001 &&
                    result.actual_variance / result.expected_variance > 0.8 &&
                    result.actual_variance / result.expected_variance < 1.2;

    return result;
}

// ============================================================================
// Collision Analysis
// ============================================================================
//
// Compares actual collision rate to birthday paradox expectation.
//
// For n random values from a space of size 2^b:
// Expected collisions ≈ n²/(2 × 2^b)
//
// For 64-bit hash with 1M samples: expected ≈ 0.03 collisions
// We use more samples to get measurable collision rates.
//
// ============================================================================

struct CollisionResult {
    std::size_t total_hashes;
    std::size_t unique_hashes;
    std::size_t collisions;
    double collision_rate;            // collisions / total
    double expected_collisions;       // Birthday paradox expectation
    double collision_ratio;           // actual / expected
    bool passed;

    void print() const {
        std::cout << "=== Collision Analysis ===\n";
        std::cout << "Total hashes:            " << total_hashes << "\n";
        std::cout << "Unique hashes:           " << unique_hashes << "\n";
        std::cout << "Collisions:              " << collisions << "\n";
        std::cout << "Collision rate:          " << std::scientific << std::setprecision(4)
                  << collision_rate << "\n";
        std::cout << "Expected (birthday):     " << std::fixed << expected_collisions << "\n";
        std::cout << "Ratio (actual/expected): " << std::setprecision(2)
                  << collision_ratio << "\n";
        std::cout << "Result:                  " << (passed ? "PASS" : "FAIL") << "\n";
    }
};

template<typename HashFn>
CollisionResult analyze_collisions(HashFn hash_fn, std::size_t num_samples = 10000000) {
    std::mt19937_64 rng(42);
    std::unordered_set<uint64_t> seen;
    seen.reserve(num_samples);

    CollisionResult result;
    result.total_hashes = num_samples;
    result.collisions = 0;

    for (std::size_t i = 0; i < num_samples; ++i) {
        uint64_t input = rng();
        uint64_t h = hash_fn(&input, sizeof(input));

        if (!seen.insert(h).second) {
            result.collisions++;
        }
    }

    result.unique_hashes = seen.size();
    result.collision_rate = static_cast<double>(result.collisions) / num_samples;

    // Birthday paradox: E[collisions] ≈ n²/(2×2^64)
    double n = num_samples;
    result.expected_collisions = (n * n) / (2.0 * static_cast<double>(1ULL << 63) * 2.0);

    // Avoid division by zero for small expected values
    if (result.expected_collisions < 0.001) {
        result.collision_ratio = (result.collisions == 0) ? 1.0 :
                                  result.collisions / 0.001;
    } else {
        result.collision_ratio = result.collisions / result.expected_collisions;
    }

    // Pass if within 10x of expected (hash space utilization is reasonable)
    result.passed = result.collision_ratio < 10.0;

    return result;
}

// ============================================================================
// Differential Analysis
// ============================================================================
//
// Tests how well the hash handles similar/related inputs:
// 1. Sequential integers (worst case for many hashes)
// 2. Strings differing by one character
// 3. Inputs with low Hamming distance
//
// Good hashes should produce uncorrelated outputs for related inputs.
//
// ============================================================================

struct DifferentialResult {
    // Sequential integer test
    double sequential_avalanche;      // Average bits changed between h(n) and h(n+1)
    double sequential_bias;           // |avalanche - 0.5|

    // Low Hamming distance test
    double hamming1_avalanche;        // Inputs differing by 1 bit
    double hamming2_avalanche;        // Inputs differing by 2 bits

    // Bit pattern test: does output depend on specific bit patterns?
    double high_bits_avalanche;       // Changes in high bits of input
    double low_bits_avalanche;        // Changes in low bits of input

    bool passed;

    void print() const {
        std::cout << "=== Differential Analysis ===\n";
        std::cout << "Sequential avalanche:    " << std::fixed << std::setprecision(4)
                  << sequential_avalanche << " (ideal: 0.5000)\n";
        std::cout << "Sequential bias:         " << sequential_bias << "\n";
        std::cout << "Hamming-1 avalanche:     " << hamming1_avalanche << "\n";
        std::cout << "Hamming-2 avalanche:     " << hamming2_avalanche << "\n";
        std::cout << "High bits avalanche:     " << high_bits_avalanche << "\n";
        std::cout << "Low bits avalanche:      " << low_bits_avalanche << "\n";
        std::cout << "Result:                  " << (passed ? "PASS" : "FAIL") << "\n";
    }
};

template<typename HashFn>
DifferentialResult analyze_differential(HashFn hash_fn, std::size_t num_samples = 100000) {
    std::mt19937_64 rng(42);
    DifferentialResult result{};

    // Sequential integer test
    {
        double total_avalanche = 0;
        uint64_t prev_hash = hash_fn("\0\0\0\0\0\0\0\0", 8);

        for (std::size_t i = 1; i <= num_samples; ++i) {
            uint64_t input = i;
            uint64_t h = hash_fn(&input, sizeof(input));

            int bits_changed = popcount(h ^ prev_hash);
            total_avalanche += static_cast<double>(bits_changed) / HASH_BITS;
            prev_hash = h;
        }

        result.sequential_avalanche = total_avalanche / num_samples;
        result.sequential_bias = std::abs(result.sequential_avalanche - 0.5);
    }

    // Hamming distance tests
    {
        double hamming1_total = 0, hamming2_total = 0;
        std::size_t hamming1_count = 0, hamming2_count = 0;

        for (std::size_t i = 0; i < num_samples; ++i) {
            uint64_t input = rng();
            uint64_t base_hash = hash_fn(&input, sizeof(input));

            // Hamming-1: flip single bit
            int bit1 = rng() % 64;
            uint64_t h1_input = input ^ (1ULL << bit1);
            uint64_t h1 = hash_fn(&h1_input, sizeof(h1_input));
            hamming1_total += static_cast<double>(popcount(base_hash ^ h1)) / HASH_BITS;
            hamming1_count++;

            // Hamming-2: flip two bits
            int bit2 = rng() % 64;
            uint64_t h2_input = input ^ (1ULL << bit1) ^ (1ULL << bit2);
            uint64_t h2 = hash_fn(&h2_input, sizeof(h2_input));
            hamming2_total += static_cast<double>(popcount(base_hash ^ h2)) / HASH_BITS;
            hamming2_count++;
        }

        result.hamming1_avalanche = hamming1_total / hamming1_count;
        result.hamming2_avalanche = hamming2_total / hamming2_count;
    }

    // High vs low bits test
    {
        double high_total = 0, low_total = 0;

        for (std::size_t i = 0; i < num_samples; ++i) {
            uint64_t input = rng();
            uint64_t base_hash = hash_fn(&input, sizeof(input));

            // Change only high bits (bits 32-63)
            int high_bit = 32 + (rng() % 32);
            uint64_t high_input = input ^ (1ULL << high_bit);
            uint64_t h_high = hash_fn(&high_input, sizeof(high_input));
            high_total += static_cast<double>(popcount(base_hash ^ h_high)) / HASH_BITS;

            // Change only low bits (bits 0-31)
            int low_bit = rng() % 32;
            uint64_t low_input = input ^ (1ULL << low_bit);
            uint64_t h_low = hash_fn(&low_input, sizeof(low_input));
            low_total += static_cast<double>(popcount(base_hash ^ h_low)) / HASH_BITS;
        }

        result.high_bits_avalanche = high_total / num_samples;
        result.low_bits_avalanche = low_total / num_samples;
    }

    // Pass if all avalanche values are close to 0.5
    result.passed = result.sequential_bias < 0.05 &&
                    std::abs(result.hamming1_avalanche - 0.5) < 0.05 &&
                    std::abs(result.hamming2_avalanche - 0.5) < 0.05 &&
                    std::abs(result.high_bits_avalanche - 0.5) < 0.05 &&
                    std::abs(result.low_bits_avalanche - 0.5) < 0.05;

    return result;
}

// ============================================================================
// Permutation Test (Sparse Key Test)
// ============================================================================
//
// Tests hash behavior on sparse inputs where only a few bits are set.
// Many weak hashes fail on inputs like {1, 2, 4, 8, 16, ...}.
//
// ============================================================================

struct PermutationResult {
    double sparse_collision_rate;     // Collisions among sparse inputs
    double two_bit_avalanche;         // Avalanche between 2-bit sparse inputs
    std::size_t total_sparse_inputs;
    std::size_t unique_hashes;
    bool passed;

    void print() const {
        std::cout << "=== Permutation/Sparse Key Test ===\n";
        std::cout << "Sparse inputs tested:    " << total_sparse_inputs << "\n";
        std::cout << "Unique hashes:           " << unique_hashes << "\n";
        std::cout << "Sparse collision rate:   " << std::fixed << std::setprecision(6)
                  << sparse_collision_rate << "\n";
        std::cout << "Two-bit avalanche:       " << std::setprecision(4)
                  << two_bit_avalanche << " (ideal: 0.5000)\n";
        std::cout << "Result:                  " << (passed ? "PASS" : "FAIL") << "\n";
    }
};

template<typename HashFn>
PermutationResult analyze_permutation(HashFn hash_fn) {
    PermutationResult result{};
    std::unordered_set<uint64_t> hashes;
    std::vector<std::pair<uint64_t, uint64_t>> two_bit_inputs;

    // Single bit inputs
    for (int i = 0; i < 64; ++i) {
        uint64_t input = 1ULL << i;
        hashes.insert(hash_fn(&input, sizeof(input)));
    }

    // Two bit inputs (all pairs)
    for (int i = 0; i < 64; ++i) {
        for (int j = i + 1; j < 64; ++j) {
            uint64_t input = (1ULL << i) | (1ULL << j);
            uint64_t h = hash_fn(&input, sizeof(input));
            hashes.insert(h);
            two_bit_inputs.emplace_back(input, h);
        }
    }

    // Three bit inputs (sample)
    for (int i = 0; i < 64; ++i) {
        for (int j = i + 1; j < 64; j += 4) {
            for (int k = j + 1; k < 64; k += 4) {
                uint64_t input = (1ULL << i) | (1ULL << j) | (1ULL << k);
                hashes.insert(hash_fn(&input, sizeof(input)));
            }
        }
    }

    result.total_sparse_inputs = 64 + (64 * 63 / 2) + (64 * 16 * 16 / 2);  // Approximate
    result.unique_hashes = hashes.size();
    result.sparse_collision_rate = 1.0 - static_cast<double>(result.unique_hashes) /
                                          result.total_sparse_inputs;

    // Two-bit avalanche: compare adjacent two-bit patterns
    double avalanche_sum = 0;
    int comparisons = 0;
    for (std::size_t i = 0; i + 1 < two_bit_inputs.size(); ++i) {
        // Compare hashes of inputs that differ by one bit position
        uint64_t diff = two_bit_inputs[i].first ^ two_bit_inputs[i + 1].first;
        if (popcount(diff) == 2) {  // They share one bit
            uint64_t hash_diff = two_bit_inputs[i].second ^ two_bit_inputs[i + 1].second;
            avalanche_sum += static_cast<double>(popcount(hash_diff)) / HASH_BITS;
            comparisons++;
        }
    }
    result.two_bit_avalanche = (comparisons > 0) ? avalanche_sum / comparisons : 0.5;

    result.passed = result.sparse_collision_rate < 0.001 &&
                    std::abs(result.two_bit_avalanche - 0.5) < 0.1;

    return result;
}

// ============================================================================
// Combined Quality Report
// ============================================================================

struct QualityReport {
    std::string hash_name;
    AvalancheResult avalanche;
    BICResult bit_independence;
    ChiSquaredResult distribution;
    CollisionResult collisions;
    DifferentialResult differential;
    PermutationResult permutation;

    int tests_passed() const {
        return avalanche.passed + bit_independence.passed + distribution.passed +
               collisions.passed + differential.passed + permutation.passed;
    }

    void print() const {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "HASH QUALITY REPORT: " << hash_name << "\n";
        std::cout << std::string(60, '=') << "\n\n";

        avalanche.print();
        std::cout << "\n";
        bit_independence.print();
        std::cout << "\n";
        distribution.print();
        std::cout << "\n";
        collisions.print();
        std::cout << "\n";
        differential.print();
        std::cout << "\n";
        permutation.print();

        std::cout << "\n" << std::string(60, '-') << "\n";
        std::cout << "OVERALL: " << tests_passed() << "/6 tests passed\n";
        std::cout << std::string(60, '=') << "\n";
    }
};

template<typename HashFn>
QualityReport full_quality_analysis(const std::string& name, HashFn hash_fn) {
    QualityReport report;
    report.hash_name = name;

    std::cout << "Analyzing " << name << "...\n";

    std::cout << "  [1/6] Avalanche analysis...\n";
    report.avalanche = analyze_avalanche(hash_fn);

    std::cout << "  [2/6] Bit independence...\n";
    report.bit_independence = analyze_bit_independence(hash_fn);

    std::cout << "  [3/6] Distribution test...\n";
    report.distribution = analyze_distribution(hash_fn);

    std::cout << "  [4/6] Collision analysis...\n";
    report.collisions = analyze_collisions(hash_fn);

    std::cout << "  [5/6] Differential analysis...\n";
    report.differential = analyze_differential(hash_fn);

    std::cout << "  [6/6] Permutation test...\n";
    report.permutation = analyze_permutation(hash_fn);

    return report;
}

} // namespace hash_quality
