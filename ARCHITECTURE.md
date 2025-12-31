# mirror_hash Architecture

This document describes the architecture and design decisions for the mirror_hash library.

## Overview

mirror_hash is a header-only C++ hashing library with two main components:

1. **Reflection-based hash** (`mirror_hash/mirror_hash.hpp`) - C++26 reflection-based automatic hashing for arbitrary types
2. **Fast hash** (`mirror_hash_unified.hpp`) - High-performance hash optimized for ARM64 with AES crypto extensions

## Repository Structure

```
include/
  mirror_hash/
    mirror_hash.hpp          # Reflection-based hash (C++26, 106KB)
  mirror_hash_unified.hpp    # Fast hash with AES optimization (C++17)

benchmarks/
  benchmark.cpp              # Benchmark: mirror_hash vs rapidhash

docs/
  METHODOLOGY.md             # Optimization methodology
  SCIENTIFIC_REPORT.md       # Detailed benchmark results
  ANALYSIS.md                # Technical analysis

profiling/
  benchmarks/                # Benchmark result files
  smhasher/                  # SMHasher test results
  assembly/                  # Assembly analysis

third_party/
  smhasher/                  # SMHasher + rapidhash for benchmarking

smhasher_upstream/           # SMHasher integration for testing
```

---

## Component 1: Reflection-Based Hash (C++26)

**Header:** `include/mirror_hash/mirror_hash.hpp`

Uses C++26 reflection (P2996) to automatically generate hash functions for arbitrary user-defined types.

### Features

- **Zero boilerplate**: Hash any struct without writing custom hash functions
- **Pluggable policies**: wyhash, Folly, MurmurHash3, xxHash3, rapidhash
- **Compile-time optimization**: Size-specialized paths when struct size is known
- **Standard compatible**: `hasher<>` adapter for `std::unordered_map/set`

### Quick Example

```cpp
#include "mirror_hash/mirror_hash.hpp"

struct Point { int x, y; };

// Hash any struct automatically
auto h = mirror_hash::hash(Point{10, 20});

// Use with standard containers
std::unordered_set<Point, mirror_hash::hasher<>> points;
```

### How It Works

The reflection-based hash iterates over struct members at compile time:

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

### Requirements

- Clang with P2996 reflection support (Bloomberg's clang-p2996 branch)
- C++26 mode (`-std=c++2c -freflection -freflection-latest`)

---

## Component 2: Fast Hash with AES Optimization

**Header:** `include/mirror_hash_unified.hpp`

A high-performance hash function that leverages ARM64 AES crypto extensions for medium-sized inputs.

### Design Strategy

The fast hash uses a size-based dispatch strategy:

| Size Range | Implementation | Rationale |
|------------|----------------|-----------|
| 0-16 bytes | rapidhashNano | Setup overhead dominates; matches rapidhash performance |
| 17-63 bytes | AES-based | Transition zone; rapidhash is actually faster here due to AES setup cost |
| 64-8192 bytes | AES-based (ARM64) | AES wins by 19-146% over rapidhash |
| >8192 bytes | rapidhash bulk | Memory bandwidth limited; CPU mixing not the bottleneck |

Note: The 24-48 byte range is a known weakness where rapidhash outperforms by 21-53%.

### Core Insight

On ARM64 with AES crypto extensions, the `AESE + AESMC` instruction pair provides a fast 16-byte mixing primitive:
- ~2 cycles for 16 bytes of mixing
- Compared to ~4-5 cycles for 128-bit multiply used by rapidhash

### API

```cpp
#include "mirror_hash_unified.hpp"

// Primary API - auto-selects optimal implementation
uint64_t h = mirror_hash::hash(data, len);
uint64_t h = mirror_hash::hash(data, len, seed);

// Explicit variant functions
uint64_t h = mirror_hash::hash_nano(data, len);   // Always rapidhashNano
uint64_t h = mirror_hash::hash_micro(data, len);  // AES when available
uint64_t h = mirror_hash::hash_bulk(data, len);   // Always rapidhash bulk
```

### Platform Support

| Platform | AES Support | Behavior |
|----------|-------------|----------|
| ARM64 with crypto | Yes | Uses AES for 64-8192 byte inputs (19-146% faster) |
| ARM64 without crypto | No | Falls back to rapidhash |
| x86-64 | No (TODO) | Falls back to rapidhash |

### Performance Results (M3 Max Pro MacBook)

| Size | rapidhash | mirror_hash | Speedup |
|------|-----------|-------------|---------|
| 8B | 1.62 ns | 1.69 ns | ~EVEN |
| 64B | 2.55 ns | 2.51 ns | +2% |
| 128B | 4.34 ns | 3.07 ns | +41% |
| 256B | 6.70 ns | 4.83 ns | +39% |
| 512B | 11.77 ns | 6.45 ns | +82% |
| 1KB | 21.35 ns | 10.19 ns | +110% |
| 4KB | 81.44 ns | 32.96 ns | +147% |
| 8KB | 149.71 ns | 63.63 ns | +135% |

### Requirements

- C++17 or later
- ARM64: `-march=armv8-a+crypto` for AES support
- Requires rapidhash.h (included via third_party/smhasher)

---

## Hash Quality

All implementations pass SMHasher tests:

| Component | SMHasher Status |
|-----------|-----------------|
| rapidhashNano (0-16B) | PASS |
| AES-based (17-8192B) | PASS |
| rapidhash bulk (>8KB) | PASS |

The AES path achieves excellent avalanche properties:
- 33/64 bits changed on single-bit input flip (ideal: 32/64)
- Consistent, deterministic output
- No detectable bias patterns

---

## Building

### Standalone Benchmark (C++17)

```bash
# ARM64 with AES
clang++ -std=c++17 -O3 -march=armv8-a+crypto \
    -I./include -I./third_party/smhasher \
    benchmarks/benchmark.cpp -o benchmark

# x86_64 (no AES)
clang++ -std=c++17 -O3 -march=x86-64 \
    -I./include -I./third_party/smhasher \
    benchmarks/benchmark.cpp -o benchmark
```

### Full Project (C++26 Reflection)

```bash
mkdir build && cd build
cmake -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE=Release \
    ..
ninja
```

---

## Design Decisions

### Why Hybrid Approach?

No single implementation wins across all sizes:
- **Small inputs**: Setup overhead dominates; simpler code wins
- **Medium inputs**: CPU mixing speed matters; AES excels
- **Large inputs**: Memory bandwidth dominates; CPU algorithm irrelevant

### Why AES for Medium Inputs?

The AESE+AESMC instruction pair on ARM64:
- Provides excellent 128-bit mixing in ~2 cycles
- Can process 4 lanes in parallel (4-way SIMD)
- No dependency chain between parallel states

### Why rapidhash for Small/Very Large?

- **Small (0-16B)**: rapidhash uses only 1-2 multiplies; AES setup overhead is too high
- **24-48B transition zone**: AES setup cost exceeds benefit; rapidhash wins by 21-53%
- **Very large (>8KB)**: Both are memory-bandwidth limited; simpler code is better

---

## References

1. [P2996: Reflection for C++26](https://wg21.link/p2996)
2. [rapidhash](https://github.com/Nicoshev/rapidhash)
3. [wyhash](https://github.com/wangyi-fudan/wyhash)
4. [SMHasher](https://github.com/rurban/smhasher)
