#include "mirror_hash/mirror_hash.hpp"
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <random>
#include <iomanip>

// ============================================================================
// High-Performance Hash Table Considerations
// ============================================================================
//
// This example explores how mirror_hash can integrate with high-performance
// hash tables like Abseil's flat_hash_map or Meta's F14.
//
// Key insights:
// 1. These tables often use a different hash combination strategy
// 2. They may benefit from "transparent" hashing (heterogeneous lookup)
// 3. Some prefer the hash to have certain statistical properties
//
// For actual integration with Abseil/Folly, you would:
// - Install abseil-cpp or folly
// - Use their hash combining functions
// - Implement AbslHashValue or a custom hasher
//
// ============================================================================

struct User {
    int id;
    std::string username;
    std::string email;
    int age;

    bool operator==(const User& other) const = default;
};

struct Transaction {
    int64_t id;
    int user_id;
    double amount;
    std::string currency;
    int64_t timestamp;

    bool operator==(const Transaction& other) const = default;
};

// Abseil-style hash integration (mock)
// In real code: template<typename H> friend H AbslHashValue(H h, const User& u)
namespace abseil_style {

template<typename T>
struct Hash {
    using is_transparent = void;

    std::size_t operator()(const T& value) const {
        // Abseil uses a different mixing strategy
        // Here we simulate it with our fast_hash
        return mirror_hash::hash(value);
    }
};

} // namespace abseil_style

// Folly/F14-style hash integration (mock)
// F14 prefers hashes with specific entropy properties
namespace folly_style {

template<typename T>
struct Hash {
    std::size_t operator()(const T& value) const noexcept {
        // F14 benefits from hashes where bits are well-mixed
        auto h = mirror_hash::hash(value);
        // Additional mixing for F14's SIMD probing
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
        return h;
    }
};

} // namespace folly_style

// Benchmark helper
template<typename Map, typename KeyGen>
void benchmark_map(const std::string& name, KeyGen gen, std::size_t n) {
    Map map;
    map.reserve(n);

    auto start = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < n; ++i) {
        auto key = gen(i);
        map[key] = static_cast<int>(i);
    }
    auto mid = std::chrono::high_resolution_clock::now();

    std::size_t found = 0;
    for (std::size_t i = 0; i < n; ++i) {
        auto key = gen(i);
        found += map.count(key);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto insert_ns = std::chrono::duration<double, std::nano>(mid - start).count() / n;
    auto lookup_ns = std::chrono::duration<double, std::nano>(end - mid).count() / n;

    std::cout << std::left << std::setw(40) << name
              << "insert: " << std::right << std::setw(8) << std::fixed << std::setprecision(1)
              << insert_ns << " ns  "
              << "lookup: " << std::setw(8) << lookup_ns << " ns  "
              << "(found: " << found << ")\n";
}

int main() {
    std::cout << "=== Hash Table Integration Examples ===\n\n";

    const std::size_t N = 100000;

    // User generator
    auto user_gen = [](std::size_t i) {
        return User{
            static_cast<int>(i),
            "user_" + std::to_string(i),
            "user" + std::to_string(i) + "@example.com",
            static_cast<int>(20 + (i % 50))
        };
    };

    // Transaction generator
    auto tx_gen = [](std::size_t i) {
        return Transaction{
            static_cast<int64_t>(i),
            static_cast<int>(i % 1000),
            static_cast<double>(i) * 0.01,
            (i % 3 == 0) ? "USD" : ((i % 3 == 1) ? "EUR" : "GBP"),
            static_cast<int64_t>(1700000000 + i)
        };
    };

    std::cout << "--- User Map (N=" << N << ") ---\n";

    benchmark_map<std::unordered_map<User, int, mirror_hash::std_hash_adapter<User>>>(
        "std::unordered_map + mirror_hash",
        user_gen, N
    );

    benchmark_map<std::unordered_map<User, int, abseil_style::Hash<User>>>(
        "std::unordered_map + abseil_style",
        user_gen, N
    );

    benchmark_map<std::unordered_map<User, int, folly_style::Hash<User>>>(
        "std::unordered_map + folly_style",
        user_gen, N
    );

    std::cout << "\n--- Transaction Map (N=" << N << ") ---\n";

    benchmark_map<std::unordered_map<Transaction, int, mirror_hash::std_hash_adapter<Transaction>>>(
        "std::unordered_map + mirror_hash",
        tx_gen, N
    );

    benchmark_map<std::unordered_map<Transaction, int, abseil_style::Hash<Transaction>>>(
        "std::unordered_map + abseil_style",
        tx_gen, N
    );

    benchmark_map<std::unordered_map<Transaction, int, folly_style::Hash<Transaction>>>(
        "std::unordered_map + folly_style",
        tx_gen, N
    );

    std::cout << "\n";
    std::cout << "Note: For true Abseil/Folly comparison, install those libraries and\n";
    std::cout << "use absl::flat_hash_map or folly::F14FastMap with their native hashers.\n";
    std::cout << "\n";
    std::cout << "The key insight is that mirror_hash provides the *automatic hashing*\n";
    std::cout << "which can then feed into any hash table's mixing/probing strategy.\n";

    return 0;
}
