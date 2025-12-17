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

mirror_hash is benchmarked against official implementations of industry-standard hash functions:

- **wyhash v4.2**: Default hash in Go, Zig, Nim
- **rapidhash V3**: Optimized variant of wyhash
- **xxHash 0.8.3**: Widely used fast hash
- **clhash**: CLMUL-based hash (x86 only)

### Benchmark Results

Using `mirror_hash_fixed` (compile-time size optimization) vs best competitor:

| Size | mirror_hash | Best Competitor | Result |
|------|-------------|-----------------|--------|
| 8B | 0.49ns | rapidhash 0.56ns | **+15%** |
| 16B | 0.47ns | rapidhash 0.62ns | **+32%** |
| 32B | 0.76ns | wyhash 0.93ns | **+22%** |
| 64B | 1.37ns | wyhash 1.61ns | **+17%** |
| 96B | 2.18ns | wyhash 2.55ns | **+17%** |
| 128B | 2.65ns | wyhash 2.94ns | **+11%** |
| 256B | 5.84ns | rapidhash 6.28ns | **+8%** |
| 512B | 11.40ns | rapidhash 11.79ns | **+3%** |
| 1024B | 20.63ns | rapidhash 21.61ns | **+5%** |

**Summary**: Wins on ALL 9 sizes tested, with significant margins on medium sizes.

Run benchmarks:
```bash
./build/official_comparison
```

## Hash Policies

| Policy | Quality | Speed | Best For |
|--------|---------|-------|----------|
| `wyhash_policy` | Excellent | **Fastest** | Default choice, maximum speed |
| `folly_policy` | Excellent | Fast | Production-proven (Meta's F14) |
| `murmur3_policy` | Excellent | Medium | Maximum portability |
| `xxhash3_policy` | Excellent | Fast | SIMD-friendly workloads |
| `rapidhash_policy` | Excellent | Fast | Large data |

All recommended policies pass avalanche, bit independence, and distribution tests.

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
