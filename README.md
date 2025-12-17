# reflect_hash

A header-only C++26 library that leverages compile-time reflection (P2996) to automatically generate hash functions for arbitrary types.

## Features

- **Zero boilerplate**: Automatically hash any struct/class without manual implementation
- **Reflection-based**: Uses C++26 reflection to iterate over members at compile time
- **Pluggable algorithms**: FNV-1a, xxHash-inspired fast hash, or bring your own
- **Standard compatible**: `std_hash_adapter` for use with `std::unordered_map/set`
- **Comprehensive type support**: Primitives, strings, containers, optionals, variants, nested types, private members

## Requirements

- Clang with P2996 reflection support (Bloomberg's clang-p2996 branch)
- libc++ with reflection enabled
- C++26 mode (`-std=c++2c -freflection -freflection-latest`)

## Quick Start

```cpp
#include "reflect_hash/reflect_hash.hpp"
#include <unordered_set>

struct Point {
    int x, y;
    bool operator==(const Point&) const = default;
};

int main() {
    // Direct hashing
    Point p{10, 20};
    std::size_t h = reflect_hash::hash(p);

    // Use with standard containers
    std::unordered_set<Point, reflect_hash::std_hash_adapter<Point>> points;
    points.insert({1, 2});
    points.insert({3, 4});
}
```

## Building

### Using the Docker Environment

```bash
# Start the development container (uses mirror_bridge image if available)
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

## API Reference

### Core Functions

```cpp
// Hash any type using default algorithm (fast_hash)
template<typename T>
std::size_t hash(const T& value);

// Hash with specific algorithm
template<typename T>
std::size_t hash_fnv1a(const T& value);

template<typename T>
std::size_t hash_fast(const T& value);

// Combine multiple values
template<typename... Args>
std::size_t hash_combine(const Args&... args);
```

### std::hash Adapter

```cpp
// For use with std::unordered_map/set
template<typename T, HashAlgorithm H = fast_hash>
struct std_hash_adapter;

// Usage:
std::unordered_map<MyType, int, reflect_hash::std_hash_adapter<MyType>> map;
```

### Custom Algorithms

Implement the `HashAlgorithm` concept:

```cpp
class my_hash {
public:
    void update(const void* data, std::size_t len);
    std::size_t finalize() const;
    static std::size_t combine(std::size_t a, std::size_t b);
};

// Use with hash<T, my_hash>(value)
```

## Supported Types

| Type | Support |
|------|---------|
| Arithmetic (int, double, etc.) | ✓ |
| Enums | ✓ |
| std::string, std::string_view | ✓ |
| C-strings | ✓ |
| std::vector, std::array, std::list, etc. | ✓ |
| std::optional | ✓ |
| std::variant | ✓ |
| std::pair | ✓ |
| std::unique_ptr, std::shared_ptr | ✓ |
| User-defined structs/classes | ✓ |
| Nested types | ✓ |
| Private members | ✓ (via unchecked access) |

## Benchmarks

Run benchmarks inside the container:

```bash
./build/benchmark_hash
```

The benchmark compares:
- `reflect_hash::fast_hash` (default, xxHash-inspired)
- `reflect_hash::fnv1a` (simple, portable)
- Manual hash implementations (baseline)

## How It Works

The core implementation uses C++26 reflection to iterate over struct members:

```cpp
template<HashAlgorithm H, Reflectable T>
void hash_append(H& algo, const T& value) {
    constexpr auto ctx = std::meta::access_context::unchecked();
    template for (constexpr auto member : std::meta::nonstatic_data_members_of(^^T, ctx)) {
        hash_append(algo, value.[:member:]);
    }
}
```

This generates optimal code at compile time - no runtime reflection overhead.

## Blog Post Topics

This project explores several interesting areas for potential blog posts:

1. **Hash function selection**: When to use FNV-1a vs xxHash vs cryptographic hashes
2. **Reflection-based generic programming**: Beyond hashing - serialization, comparison, etc.
3. **Performance analysis**: Comparing compile-time reflection to manual implementations
4. **Hash table integration**: Working with Abseil, Folly, or other high-performance containers

## License

MIT
