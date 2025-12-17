#include "mirror_hash/mirror_hash.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <string>
#include <array>

// ============================================================================
// Policy Comparison Benchmark
// ============================================================================
//
// Compares all hash policies across different data types and sizes.
// Reports both quality metrics and performance numbers.
//
// ============================================================================

inline void do_not_optimize(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

// Test structures
struct SmallStruct {
    int x, y;
    bool operator==(const SmallStruct&) const = default;
};

struct MediumStruct {
    int a, b, c, d;
    double e, f;
    char g;
};

struct LargeStruct {
    std::array<int, 16> data;
    double values[4];
    std::string name;
};

struct WithVector {
    std::vector<int> small_vec;
    std::vector<double> large_vec;
};

// Data generation
std::mt19937_64 rng(42);

template<typename T>
T generate();

template<>
int generate<int>() {
    return std::uniform_int_distribution<int>(-1000000, 1000000)(rng);
}

template<>
double generate<double>() {
    return std::uniform_real_distribution<double>(-1000.0, 1000.0)(rng);
}

template<>
std::string generate<std::string>() {
    static const char chars[] = "abcdefghijklmnopqrstuvwxyz";
    int len = std::uniform_int_distribution<int>(5, 50)(rng);
    std::string s;
    s.reserve(len);
    for (int i = 0; i < len; ++i) {
        s += chars[std::uniform_int_distribution<int>(0, 25)(rng)];
    }
    return s;
}

template<>
SmallStruct generate<SmallStruct>() {
    return {generate<int>(), generate<int>()};
}

template<>
MediumStruct generate<MediumStruct>() {
    return {
        generate<int>(), generate<int>(), generate<int>(), generate<int>(),
        generate<double>(), generate<double>(),
        static_cast<char>(generate<int>() % 128)
    };
}

template<>
LargeStruct generate<LargeStruct>() {
    LargeStruct s;
    for (auto& v : s.data) v = generate<int>();
    for (auto& v : s.values) v = generate<double>();
    s.name = generate<std::string>();
    return s;
}

template<>
WithVector generate<WithVector>() {
    WithVector w;
    w.small_vec.resize(100);
    w.large_vec.resize(1000);
    for (auto& v : w.small_vec) v = generate<int>();
    for (auto& v : w.large_vec) v = generate<double>();
    return w;
}

// Benchmark runner
template<typename Policy, typename T>
double benchmark_policy(const T& data, std::size_t iterations) {
    // Warmup
    std::size_t sink = 0;
    for (std::size_t i = 0; i < iterations / 10; ++i) {
        auto h = mirror_hash::hash<Policy>(data);
        do_not_optimize(&h);
        sink += h;
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        auto h = mirror_hash::hash<Policy>(data);
        do_not_optimize(&h);
        sink += h;
    }
    auto end = std::chrono::high_resolution_clock::now();

    do_not_optimize(&sink);
    return std::chrono::duration<double, std::nano>(end - start).count() / iterations;
}

// Quality check - test the policy's combine function directly
template<typename Policy>
double check_avalanche() {
    constexpr int SAMPLES = 10000;
    double total_ratio = 0;

    for (int i = 0; i < SAMPLES; ++i) {
        std::uint64_t input = rng();
        // Test the policy's combine function directly, not std::hash
        std::uint64_t base_hash = Policy::combine(0, input);

        for (int bit = 0; bit < 64; ++bit) {
            std::uint64_t flipped = input ^ (1ULL << bit);
            std::uint64_t flipped_hash = Policy::combine(0, flipped);
            int bits_changed = __builtin_popcountll(base_hash ^ flipped_hash);
            total_ratio += static_cast<double>(bits_changed) / 64.0;
        }
    }

    return total_ratio / (SAMPLES * 64);
}

// Print results in a nice table
void print_separator(int width = 100) {
    std::cout << std::string(width, '-') << "\n";
}

template<typename Policy>
void run_policy_benchmark(const char* policy_name, std::size_t iterations) {
    auto small = generate<SmallStruct>();
    auto medium = generate<MediumStruct>();
    auto large = generate<LargeStruct>();
    auto with_vec = generate<WithVector>();
    std::vector<int> vec_1k(1000);
    std::vector<int> vec_10k(10000);
    for (auto& v : vec_1k) v = generate<int>();
    for (auto& v : vec_10k) v = generate<int>();

    double t_small = benchmark_policy<Policy>(small, iterations);
    double t_medium = benchmark_policy<Policy>(medium, iterations);
    double t_large = benchmark_policy<Policy>(large, iterations);
    double t_with_vec = benchmark_policy<Policy>(with_vec, iterations / 10);
    double t_vec_1k = benchmark_policy<Policy>(vec_1k, iterations / 10);
    double t_vec_10k = benchmark_policy<Policy>(vec_10k, iterations / 100);
    double avalanche = check_avalanche<Policy>();

    std::cout << std::left << std::setw(12) << policy_name
              << std::right << std::fixed << std::setprecision(2)
              << std::setw(10) << t_small
              << std::setw(10) << t_medium
              << std::setw(10) << t_large
              << std::setw(12) << t_with_vec
              << std::setw(10) << t_vec_1k
              << std::setw(12) << t_vec_10k
              << std::setw(10) << std::setprecision(4) << avalanche
              << (std::abs(avalanche - 0.5) < 0.02 ? "  GOOD" : "  POOR")
              << "\n";
}

int main() {
    constexpr std::size_t ITERATIONS = 1000000;

    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << "       HASH POLICY COMPARISON BENCHMARK\n";
    std::cout << "============================================================\n\n";

    std::cout << "SIMD Backend: ";
    switch (mirror_hash::simd::active_implementation) {
        case mirror_hash::simd::implementation::avx512: std::cout << "AVX-512\n"; break;
        case mirror_hash::simd::implementation::avx2: std::cout << "AVX2\n"; break;
        case mirror_hash::simd::implementation::sse42: std::cout << "SSE4.2\n"; break;
        case mirror_hash::simd::implementation::neon: std::cout << "NEON\n"; break;
        default: std::cout << "Scalar\n"; break;
    }
    std::cout << "Iterations: " << ITERATIONS << "\n\n";

    // Header
    std::cout << std::left << std::setw(12) << "Policy"
              << std::right
              << std::setw(10) << "Small"
              << std::setw(10) << "Medium"
              << std::setw(10) << "Large"
              << std::setw(12) << "WithVec"
              << std::setw(10) << "Vec1k"
              << std::setw(12) << "Vec10k"
              << std::setw(10) << "Aval"
              << "  Quality\n";

    std::cout << std::left << std::setw(12) << ""
              << std::right
              << std::setw(10) << "(ns)"
              << std::setw(10) << "(ns)"
              << std::setw(10) << "(ns)"
              << std::setw(12) << "(ns)"
              << std::setw(10) << "(ns)"
              << std::setw(12) << "(ns)"
              << std::setw(10) << "(ideal=0.5)"
              << "\n";

    print_separator();

    // Run benchmarks for each policy
    run_policy_benchmark<mirror_hash::folly_policy>("folly", ITERATIONS);
    run_policy_benchmark<mirror_hash::wyhash_policy>("wyhash", ITERATIONS);
    run_policy_benchmark<mirror_hash::murmur3_policy>("murmur3", ITERATIONS);
    run_policy_benchmark<mirror_hash::xxhash3_policy>("xxhash3", ITERATIONS);
    run_policy_benchmark<mirror_hash::aes_policy>("aes", ITERATIONS);
    run_policy_benchmark<mirror_hash::fnv1a_policy>("fnv1a", ITERATIONS);

    print_separator();

    std::cout << "\nLegend:\n";
    std::cout << "  Small    = struct{int, int}\n";
    std::cout << "  Medium   = struct{4 int, 2 double, char}\n";
    std::cout << "  Large    = struct{array<int,16>, double[4], string}\n";
    std::cout << "  WithVec  = struct{vector<int>(100), vector<double>(1000)}\n";
    std::cout << "  Vec1k    = vector<int> with 1000 elements\n";
    std::cout << "  Vec10k   = vector<int> with 10000 elements\n";
    std::cout << "  Aval     = Avalanche ratio (ideal = 0.5000)\n";
    std::cout << "\n";

    // Recommendations
    std::cout << "RECOMMENDATIONS:\n";
    std::cout << "  - General purpose: folly (default) - best balance of speed and quality\n";
    std::cout << "  - Maximum speed:   wyhash - fastest, requires 128-bit multiply\n";
    std::cout << "  - Portability:     murmur3 - no special CPU features needed\n";
    std::cout << "  - SIMD-friendly:   xxhash3 - designed for vectorization\n";
    std::cout << "  - AVOID:           fnv1a - poor avalanche properties\n";
    std::cout << "\n";

    // Policy selection example
    std::cout << "USAGE EXAMPLES:\n";
    std::cout << "  // Default (Folly)\n";
    std::cout << "  auto h1 = mirror_hash::hash(my_struct);\n";
    std::cout << "\n";
    std::cout << "  // Explicit policy selection\n";
    std::cout << "  auto h2 = mirror_hash::hash<mirror_hash::wyhash_policy>(my_struct);\n";
    std::cout << "\n";
    std::cout << "  // With unordered containers\n";
    std::cout << "  std::unordered_set<MyType, mirror_hash::hasher<mirror_hash::wyhash_policy>> set;\n";
    std::cout << "\n";

    return 0;
}
