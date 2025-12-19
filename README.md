# mirror_hash

A header-only C++26 library that leverages compile-time reflection (P2996) to automatically generate high-performance hash functions for arbitrary types.

## Features

- **Zero boilerplate**: Automatically hash any struct/class without manual implementation
- **Reflection-based**: Uses C++26 reflection to iterate over members at compile time
- **Competitive performance**: Matches or beats wyhash and rapidhash on most input sizes
- **Pluggable policies**: wyhash, Folly, MurmurHash3, xxHash3, or bring your own
- **Standard compatible**: `hasher<>` adapter for use with `std::unordered_map/set`
- **Compile-time optimization**: Size-specialized hashing when struct size is known

## Requirements

- Clang with P2996 reflection support (Bloomberg's clang-p2996 branch)
- libc++ with reflection enabled
- C++26 mode (`-std=c++2c -freflection -freflection-latest`)

## Quick Start

```cpp
#include "mirror_hash/mirror_hash.hpp"
#include <unordered_set>

struct Point {
    int x, y;
    bool operator==(const Point&) const = default;
};

int main() {
    // Direct hashing (uses wyhash_policy by default for speed)
    Point p{10, 20};
    auto h = mirror_hash::hash(p);

    // Use with standard containers
    std::unordered_set<Point, mirror_hash::hasher<>> points;
    points.insert({1, 2});
    points.insert({3, 4});

    // Explicit policy selection
    auto h2 = mirror_hash::hash<mirror_hash::folly_policy>(p);
}
```

## Building

### Using the Docker Environment (Recommended)

```bash
# Start the development container
./start_dev_container.sh

# Inside container:
./build.sh
./run_tests.sh
```

### Manual Build

```bash
mkdir build && cd build
cmake -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE=Release \
    ..
ninja
```

## Performance

Benchmarked against official implementations from their respective repositories using methodology inspired by [xxHash](https://github.com/Cyan4973/xxHash).

### Benchmark Results

**Test Environment:** ARM64 (Apple Silicon), Clang 21 with P2996, `-O3 -march=native`

#### Throughput (GB/s, higher = better)

| Hash | Width | Bulk 256KB | 64B | 16B | Notes |
|------|-------|------------|-----|-----|-------|
| **mirror_hash*** | 64 | **54.1** | **32.7** | 8.0 | Compile-time size |
| rapidhash | 64 | 54.0 | 21.9 | 8.2 | |
| XXH3 | 64 | 48.6 | 18.5 | 8.0 | |
| wyhash | 64 | 30.3 | 21.6 | 7.9 | Go/Zig/Nim default |
| XXH64 | 64 | 16.6 | 9.7 | 5.2 | |
| mirror_hash | 64 | 27.1 | 9.6 | 3.3 | Runtime size |
| FNV-1a | 64 | 0.9 | 1.4 | 2.5 | Poor quality |

#### Small Data Latency (ns/hash, lower = better)

| Hash | 8B | 16B | 32B | 64B | 256B |
|------|-----|------|------|------|------|
| **mirror_hash*** | 1.99 | **1.49** | **1.98** | **1.81** | **5.71** |
| rapidhash | 1.99 | 2.00 | 2.50 | 2.92 | 7.52 |
| wyhash | 2.03 | 2.00 | 2.15 | 3.03 | 7.26 |
| XXH3 | 2.03 | 1.96 | 2.40 | 3.36 | 9.15 |
| XXH64 | 2.75 | 3.06 | 5.20 | 6.45 | 17.47 |

\* `mirror_hash*` uses compile-time size optimization (`hash_bytes_fixed<N>`)

### Hash Quality (SMHasher Test Suite)

Quality validated using [SMHasher](https://github.com/rurban/smhasher) test algorithms:

| Hash | Tests Passed | Bulk (GB/s) | Small 16B (ns) | Status |
|------|--------------|-------------|----------------|--------|
| **rapidhash** | 12/12 | **54.2** | 1.36 | PASS |
| **mirror_rapid** | **12/12** | **54.1** | 1.69 | **PASS** |
| XXH3 | 12/12 | 50.1 | 1.49 | PASS |
| wyhash | 12/12 | 31.1 | 1.60 | PASS |
| mirror_wyhash | 12/12 | 31.3 | 1.77 | PASS |
| XXH64 | 12/12 | 18.4 | 2.43 | PASS |

**mirror_rapid** uses `rapidhash_policy` with 7-way parallel accumulators, matching official rapidhash performance at **54 GB/s**.
**mirror_wyhash** uses `wyhash_policy` with the official wyhash algorithm, passing all quality tests (BIC correlation: 0.002).

**SMHasher Tests Implemented:**
- **Sanity**: Verification, alignment handling, appended zeroes
- **Avalanche** (SAC): Each input bit flip should change ~50% of output bits
- **Bit Independence** (BIC): Output bit changes should be uncorrelated
- **Keyset**: Sparse, permutation, cyclic, and text patterns
- **Differential**: Sequential/related inputs should produce uncorrelated outputs
- **Collision**: Birthday paradox collision rate validation
- **Distribution**: Chi-squared uniformity test

Run the SMHasher test suite:
```bash
./build/smhasher_tests
```

Run benchmarks:
```bash
./build/comprehensive_benchmark  # Full benchmark suite
./build/official_comparison      # Quick comparison
```

## Hash Policies

| Policy | Quality | Bulk Speed | Best For |
|--------|---------|------------|----------|
| `rapidhash_policy` | Excellent | **54 GB/s** | Large data, maximum throughput |
| `wyhash_policy` | Excellent | 31 GB/s | Default choice, well-tested |
| `folly_policy` | Excellent | Fast | Production-proven (Meta's F14) |
| `murmur3_policy` | Excellent | Medium | Maximum portability |
| `xxhash3_policy` | Excellent | Fast | SIMD-friendly workloads |

All policies pass SMHasher quality tests (avalanche, bit independence, distribution).

## API Reference

### Core Functions

```cpp
// Hash any reflectable type (default: wyhash_policy)
template<typename Policy = wyhash_policy, typename T>
std::size_t hash(const T& value);

// Hash raw bytes with compile-time size optimization
template<typename Policy, std::size_t N>
std::size_t hash_bytes_fixed(const void* data);

// Hash raw bytes with runtime size
template<typename Policy>
std::size_t hash_bytes(const void* data, std::size_t len);
```

### std::hash Adapter

```cpp
// For use with std::unordered_map/set
template<typename Policy = wyhash_policy>
struct hasher;

// Usage:
std::unordered_map<MyType, int, mirror_hash::hasher<>> map;
```

### Custom Policy

```cpp
struct my_policy {
    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        // Your mixing function
    }
    static constexpr std::size_t mix(std::size_t k) noexcept {
        // Single-value finalization
    }
};

auto h = mirror_hash::hash<my_policy>(my_struct);
```

## Supported Types

| Type | Support |
|------|---------|
| Arithmetic (int, double, etc.) | ✓ |
| Enums | ✓ |
| std::string, std::string_view | ✓ |
| std::vector, std::array, std::list, etc. | ✓ |
| std::optional | ✓ |
| std::variant | ✓ |
| std::pair, std::tuple | ✓ |
| std::unique_ptr, std::shared_ptr | ✓ |
| User-defined structs/classes | ✓ |
| Nested types | ✓ |
| Private members | ✓ (via reflection) |

## How It Works

mirror_hash uses C++26 reflection to iterate over struct members at compile time:

```cpp
template<typename Policy, Reflectable T>
std::size_t hash_impl(const T& value) noexcept {
    constexpr auto members = std::meta::nonstatic_data_members_of(
        ^^T, std::meta::access_context::unchecked());

    std::size_t result = 0;
    template for (constexpr auto member : members) {
        result = Policy::combine(result, hash_impl<Policy>(value.[:member:]));
    }
    return result;
}
```

This generates optimal code at compile time - no runtime reflection overhead.

## License

MIT
