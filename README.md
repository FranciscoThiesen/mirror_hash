# mirror_hash

Automatic struct hashing using C++26 reflection. No boilerplate. No manual member enumeration.

## The Problem

Every time you use a struct in an `unordered_map`, you write this:

```cpp
struct Point { int x, y, z; };

template<>
struct std::hash<Point> {
    std::size_t operator()(const Point& p) const noexcept {
        std::size_t seed = 0;
        seed ^= std::hash<int>{}(p.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<int>{}(p.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};
```

Add a member? Update the hash. Forget one? Silent bugs.

## The Solution

```cpp
#include "mirror_hash/mirror_hash.hpp"

struct Point { int x, y, z; };

int main() {
    // Just works. No specialization needed.
    auto h = mirror_hash::hash(Point{10, 20, 30});

    // Works with standard containers
    std::unordered_set<Point, mirror_hash::hasher<>> points;
    points.insert({1, 2, 3});
}
```

C++26 reflection (P2996) lets the compiler enumerate your struct's members. mirror_hash uses this to generate hash functions automatically.

## Features

### Automatic Hashing (C++26)

- **Any struct**: No macros, no annotations, no `cista_members()` functions
- **Recursive**: Handles nested structs automatically
- **Containers**: `std::vector`, `std::string`, `std::optional`, `std::variant` all work
- **Fast path**: Trivially copyable types hash as raw bytes (zero overhead for `Point{int,int,int}`)

### High-Performance Byte Hashing (C++17)

For raw bytes, mirror_hash includes a hybrid implementation that uses the optimal algorithm for each input size:

```cpp
#include "mirror_hash_unified.hpp"

// Fast hash for any byte buffer
uint64_t h = mirror_hash::hash(data, len);
uint64_t h = mirror_hash::hash(data, len, seed);
```

On ARM64 with AES crypto extensions (Apple Silicon, AWS Graviton):
- **Small inputs (0-16B)**: Uses rapidhashNano (matches rapidhash performance)
- **Medium-large inputs (64B-8KB)**: Uses hardware AES acceleration (18-147% faster than rapidhash)
- **Very large inputs (>8KB)**: Uses rapidhash (memory bandwidth limited)

Note: The 17-48 byte range has higher overhead due to AES setup cost; rapidhash is faster there.

Falls back to rapidhash on platforms without AES support.

## Quick Start

### Reflection-Based Hashing (C++26)

Requires [clang-p2996](https://github.com/bloomberg/clang-p2996):

```bash
clang++ -std=c++2c -freflection -freflection-latest \
    -I./include mycode.cpp
```

```cpp
#include "mirror_hash/mirror_hash.hpp"

struct Person {
    std::string name;
    int age;
};

std::unordered_map<Person, Data, mirror_hash::hasher<>> people;
```

### Byte Hashing (C++17)

Works with any modern compiler:

```bash
# ARM64 with AES (Apple Silicon, Graviton)
clang++ -std=c++17 -O3 -march=armv8-a+crypto -I./include mycode.cpp

# x86-64
clang++ -std=c++17 -O3 -I./include mycode.cpp
```

```cpp
#include "mirror_hash_unified.hpp"

uint64_t hash = mirror_hash::hash(buffer, size);
```

## Project Structure

```
include/
├── mirror_hash/
│   └── mirror_hash.hpp      # C++26 reflection-based hasher
└── mirror_hash_unified.hpp  # C++17 high-performance byte hasher

benchmarks/                   # Performance benchmarks
profiling/                    # Memory bandwidth analysis
tests/                        # Unit tests
third_party/smhasher/         # Hash quality testing (rapidhash, gxhash)
```

## Building & Testing

### Using Docker (Recommended)

```bash
./start_dev_container.sh   # Starts container with clang-p2996
./build.sh                 # Builds tests and benchmarks
./run_tests.sh             # Runs test suite
```

### Manual Build

```bash
mkdir build && cd build
cmake -DCMAKE_CXX_COMPILER=clang++ ..
make -j4
./test_mirror_hash
```

## Hash Quality

Passes all [SMHasher](https://github.com/rurban/smhasher) tests:

```bash
cd smhasher_upstream/build
./SMHasher mirror_hash_unified
# All tests PASS
```

## Performance

### ARM64 with AES (Apple Silicon)

Benchmarked on M3 Max Pro MacBook with `-O3 -march=armv8-a+crypto`:

| Size | rapidhash | GxHash | mirror_hash | Notes |
|------|-----------|--------|-------------|-------|
| 8B   | 1.62 ns   | 2.21 ns| 1.69 ns     | Small: ~EVEN with rapidhash |
| 64B  | 2.55 ns   | 4.02 ns| **2.51 ns** | AES wins |
| 128B | 4.34 ns   | 3.48 ns| **3.07 ns** | Beats both |
| 256B | 6.70 ns   | 4.29 ns| **4.03 ns** | Beats GxHash by 6% |
| 512B | 11.77 ns  | 6.17 ns| **5.91 ns** | Beats GxHash by 4% |
| 1KB  | 21.35 ns  | 9.91 ns| **9.66 ns** | Beats GxHash by 3% |
| 4KB  | 81.44 ns  | 38.60 ns| **31.97 ns**| Beats GxHash by 21% |
| 8KB  | 149.64 ns | 79.42 ns| **61.50 ns**| Beats GxHash by 29% |

**Summary**: mirror_hash matches rapidhash for small inputs (0-16B), beats both rapidhash (by 18-147%) and GxHash (by 3-60%) for medium-large inputs (64B-8KB).

### Trivially Copyable Structs

| Type | Time | Notes |
|------|------|-------|
| `Point{int,int}` | ~0.3 ns | Raw bytes, zero overhead |
| `Person{string,int}` | ~2.5 ns | Per-member hashing |
| `Data{vector<int>,string}` | scales with size | Iterates elements |

## Requirements

| Feature | Compiler | Flags |
|---------|----------|-------|
| Reflection hashing | clang-p2996 | `-std=c++2c -freflection -freflection-latest` |
| Byte hashing | Any C++17 | `-std=c++17` |
| AES optimization | ARM64 w/ crypto | `-march=armv8-a+crypto` |

## Acknowledgments

Built on excellent work by:
- [rapidhash](https://github.com/Nicoshev/rapidhash) - Nicolas De Carli
- [GxHash](https://github.com/ogxd/gxhash) - ogxd
- [wyhash](https://github.com/wangyi-fudan/wyhash) - Wang Yi
- [P2996](https://wg21.link/P2996) - Wyatt Childers, Peter Dimov, Dan Katz, Barry Revzin, Andrew Sutton, Faisal Vali, Daveed Vandevoorde

## License

MIT
