// Benchmark: Person struct with different string sizes
// Run in Docker with clang-p2996 for reflection support

#include "mirror_hash/mirror_hash.hpp"
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

using Clock = std::chrono::high_resolution_clock;

// Use volatile to prevent optimization
volatile uint64_t sink = 0;

struct PersonShort {
    std::string name;
    int age;
    bool operator==(const PersonShort&) const = default;
};

struct PersonLong {
    std::string name;
    int age;
    bool operator==(const PersonLong&) const = default;
};

struct PointTrivial {
    int x, y;
    bool operator==(const PointTrivial&) const = default;
};

struct DataWithVector5 {
    std::vector<int> values;
    std::string name;
    bool operator==(const DataWithVector5&) const = default;
};

struct DataWithVector100 {
    std::vector<int> values;
    std::string name;
    bool operator==(const DataWithVector100&) const = default;
};

// Benchmark with seed variation to prevent loop hoisting
template<typename T, typename Mutator>
__attribute__((noinline))
double benchmark_with_mutation(T value, size_t iterations, Mutator mutate) {
    // Warmup
    uint64_t warmup_sum = 0;
    for (size_t i = 0; i < 1000; i++) {
        mutate(value, i);
        warmup_sum += mirror_hash::hash(value);
    }
    sink = warmup_sum;

    auto start = Clock::now();
    uint64_t sum = 0;
    for (size_t i = 0; i < iterations; i++) {
        mutate(value, i);
        sum += mirror_hash::hash(value);
    }
    auto end = Clock::now();

    sink = sum;

    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    return static_cast<double>(ns) / iterations;
}

int main() {
    constexpr size_t ITERS = 5000000;

    std::cout << "=== mirror_hash Struct Benchmark ===" << std::endl;
    std::cout << "Iterations: " << ITERS << std::endl;
    std::cout << "Note: Each iteration mutates the value to prevent loop hoisting" << std::endl;
    std::cout << std::endl;

    // Point{int, int} - trivially copyable
    double pt_ns = benchmark_with_mutation(
        PointTrivial{10, 20}, ITERS,
        [](PointTrivial& p, size_t i) { p.x = static_cast<int>(i); }
    );
    std::cout << "Point{int, int}:           " << pt_ns << " ns" << std::endl;

    // Person with 5-char string (SSO) - but we need to modify it
    // Can't easily modify string content, so modify age
    double ps_ns = benchmark_with_mutation(
        PersonShort{"Alice", 30}, ITERS,
        [](PersonShort& p, size_t i) { p.age = static_cast<int>(i % 100); }
    );
    std::cout << "Person{string(5), int}:    " << ps_ns << " ns  (SSO, ~5 chars)" << std::endl;

    // Person with 32-char string (heap)
    double pl_ns = benchmark_with_mutation(
        PersonLong{"AliceInWonderlandIsALongNameHere", 30}, ITERS,
        [](PersonLong& p, size_t i) { p.age = static_cast<int>(i % 100); }
    );
    std::cout << "Person{string(32), int}:   " << pl_ns << " ns  (heap, ~32 chars)" << std::endl;

    // Also test a 16-char string (borderline SSO)
    double pm_ns = benchmark_with_mutation(
        PersonShort{"AliceWonderland!", 30}, ITERS,  // 16 chars
        [](PersonShort& p, size_t i) { p.age = static_cast<int>(i % 100); }
    );
    std::cout << "Person{string(16), int}:   " << pm_ns << " ns  (SSO borderline)" << std::endl;

    // Data with vector(5)
    double dv5_ns = benchmark_with_mutation(
        DataWithVector5{{1, 2, 3, 4, 5}, "test"}, ITERS,
        [](DataWithVector5& d, size_t i) { d.values[0] = static_cast<int>(i); }
    );
    std::cout << "Data{vector(5), string}:   " << dv5_ns << " ns" << std::endl;

    // Data with vector(100)
    std::vector<int> v100(100);
    for (int i = 0; i < 100; i++) v100[i] = i;
    double dv100_ns = benchmark_with_mutation(
        DataWithVector100{v100, "test"}, ITERS / 5,
        [](DataWithVector100& d, size_t i) { d.values[0] = static_cast<int>(i); }
    );
    std::cout << "Data{vector(100), string}: " << dv100_ns << " ns" << std::endl;

    std::cout << std::endl;
    std::cout << "=== Ratios ===" << std::endl;
    std::cout << "string(32) / string(5) = " << pl_ns / ps_ns << "x" << std::endl;
    std::cout << "vector(100) / vector(5) = " << dv100_ns / dv5_ns << "x" << std::endl;

    // SSO check
    std::string sso_test = "Alice";
    std::string heap_test = "AliceInWonderlandIsALongNameHere";
    std::cout << std::endl;
    std::cout << "=== SSO Info (libc++) ===" << std::endl;
    std::cout << "string(5) capacity: " << sso_test.capacity() << std::endl;
    std::cout << "string(32) capacity: " << heap_test.capacity() << std::endl;

    return 0;
}
