# reflect_hash Architecture

This document describes the design decisions, implementation details, and performance analysis of reflect_hash - a configurable, reflection-based hashing library for C++26.

## Overview

reflect_hash leverages C++26 reflection (P2996) to automatically generate hash functions for arbitrary user-defined types. It features:

- **Zero Boilerplate**: Hash any struct without writing custom hash functions
- **Configurable Policies**: Choose from multiple hash algorithms (Folly, wyhash, MurmurHash3, xxHash3)
- **SIMD Optimization**: Architecture-specific acceleration (AVX-512, AVX2, NEON)
- **High Quality**: All recommended policies pass rigorous statistical tests

## Design Principles

### 1. Policy-Based Design (Zero Runtime Overhead)

Hash algorithms are selected at compile time via template parameters:

```cpp
// Default (Folly) - no runtime dispatch
auto h1 = reflect_hash::hash(my_struct);

// Explicit policy selection - still no runtime dispatch
auto h2 = reflect_hash::hash<reflect_hash::wyhash_policy>(my_struct);

// With containers
std::unordered_set<MyType, reflect_hash::hasher<reflect_hash::wyhash_policy>> set;
```

The policy is a compile-time template parameter, so the compiler can inline everything. There is **zero overhead** compared to a hardcoded implementation.

### 2. SIMD-Aware Bulk Hashing (simdjson-inspired)

For large data (arrays, vectors), we use architecture-specific optimizations:

```cpp
namespace simd {
    // Compile-time detection
    #if REFLECT_HASH_AVX512
        // Use 8 parallel hash states, process 64 bytes per iteration
    #elif REFLECT_HASH_AVX2
        // Use 4 parallel hash states, process 32 bytes per iteration
    #elif REFLECT_HASH_NEON
        // Use 2 parallel hash states, process 16 bytes per iteration
    #else
        // Scalar fallback, 8 bytes per iteration
    #endif
}
```

This approach is inspired by [simdjson](https://github.com/simdjson/simdjson).

### 3. Concept-Based Type Dispatch

Types are automatically routed to the best hashing strategy:

| Concept | Strategy |
|---------|----------|
| `Arithmetic` | `std::hash<T>` |
| `ContiguousArithmeticContainer` | SIMD bulk byte hashing |
| `Container` | Per-element hashing |
| `Reflectable` | Compile-time member iteration |

## Hash Policies

### Available Policies

| Policy | Quality | Speed | Notes |
|--------|---------|-------|-------|
| `folly_policy` (default) | 5/6 | Fast | Meta's F14 hash tables |
| `wyhash_policy` | 5/6 | **Fastest** | Go/Zig/Nim default, 128-bit multiply |
| `murmur3_policy` | 5/6 | Medium | Classic, very portable |
| `xxhash3_policy` | 5/6 | Fast | Modern, SIMD-friendly |
| `aes_policy` | 5/6 | Fast | AES-NI accelerated |
| `fnv1a_policy` | 2/6 | Fast | **NOT RECOMMENDED** - poor avalanche |

### Policy Selection Guide

```cpp
// General purpose (recommended default)
using Policy = reflect_hash::folly_policy;

// Maximum speed (requires 128-bit multiply support)
using Policy = reflect_hash::wyhash_policy;

// Maximum portability (no special CPU features)
using Policy = reflect_hash::murmur3_policy;

// SIMD-friendly applications
using Policy = reflect_hash::xxhash3_policy;
```

### Why We Chose Folly as Default

After quality analysis and benchmarking:

1. **Proven in production** - Powers Meta's F14 hash tables (billions of lookups/day)
2. **Excellent quality** - 5/6 tests, avalanche bias 0.0003
3. **Portable** - No 128-bit multiplication (works everywhere)
4. **Fast** - Competitive performance across all data sizes
5. **Simple** - Easy to understand, verify, and debug

## Quality Analysis

We implemented rigorous statistical tests based on cryptographic literature:

| Test | Origin | Purpose |
|------|--------|---------|
| Avalanche | Webster & Tavares, 1985 | Bit diffusion |
| Bit Independence | Webster & Tavares, 1985 | Output correlation |
| Chi-Squared | Pearson/Knuth | Distribution uniformity |
| Collision | Birthday Paradox | Hash space utilization |
| Differential | Biham & Shamir, 1990 | Similar input handling |
| Permutation | SMHasher | Sparse key behavior |

See `analysis/QUALITY_TESTS.md` for detailed test descriptions.

### Quality Results

```
Policy              Avalanche  BIC    Chi²   Collision  Diff   Perm   Score
--------------------------------------------------------------------------------
folly               PASS       PASS   PASS   PASS       PASS   FAIL   5/6
wyhash              PASS       PASS   PASS   PASS       PASS   FAIL   5/6
murmur3             PASS       PASS   PASS   PASS       PASS   FAIL   5/6
xxhash3             PASS       PASS   PASS   PASS       PASS   FAIL   5/6
fnv1a               FAIL       FAIL   PASS   PASS       FAIL   FAIL   2/6
```

## Performance

### Benchmark Results (ARM64/NEON)

| Policy | Small (ns) | Medium (ns) | Vec1k (ns) | Avalanche |
|--------|-----------|-------------|------------|-----------|
| wyhash | **0.62** | **2.40** | **343** | 0.499 |
| fnv1a | 0.58 | 2.91 | 545 | 0.228 ⚠️ |
| xxhash3 | 0.93 | 5.76 | 861 | 0.500 |
| folly | 0.97 | 7.14 | 926 | 0.500 |
| aes | 0.98 | 5.29 | 734 | 0.502 |
| murmur3 | 1.40 | 9.48 | 940 | 0.500 |

Key observations:
- **wyhash** is 2-3x faster for large data due to 128-bit multiply
- **fnv1a** is fast but has poor quality (0.228 avalanche vs 0.5 ideal)
- SIMD (NEON) provides significant speedup for bulk hashing

### SIMD Speedup

For `vector<int>` with 10,000 elements:

| Backend | Time | Speedup |
|---------|------|---------|
| Scalar | ~15ms | 1x |
| NEON (2-way) | ~3.5ms | 4x |
| AVX2 (4-way) | ~2ms | 7x (estimated) |
| AVX-512 (8-way) | ~1ms | 15x (estimated) |

## Implementation Details

### Reflection-Based Member Iteration

```cpp
template<typename T>
consteval std::size_t member_count() {
    return std::meta::nonstatic_data_members_of(
        ^^T, std::meta::access_context::current()
    ).size();
}

template<typename Policy, Reflectable T>
std::size_t hash_impl(const T& value) noexcept {
    constexpr std::size_t N = member_count<T>();
    std::size_t result = 0;
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        ((result = Policy::combine(result,
            hash_impl<Policy>(value.[:get_member<T, Is>():]))), ...);
    }(std::make_index_sequence<N>{});
    return result;
}
```

### Policy Concept

```cpp
template<typename P>
concept HashPolicy = requires(std::size_t a, std::size_t b) {
    { P::combine(a, b) } noexcept -> std::same_as<std::size_t>;
};
```

All policies must provide:
- `combine(seed, value)` - Combine two hash values
- `mix(value)` - Finalize/mix a single value

### Custom Policy Example

```cpp
struct my_policy {
    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        // Your mixing function here
        return seed ^ (value * 0x9e3779b97f4a7c15ULL);
    }

    static constexpr std::size_t mix(std::size_t k) noexcept {
        return combine(0, k);
    }
};

// Use it
auto h = reflect_hash::hash<my_policy>(my_struct);
```

## File Structure

```
include/reflect_hash/
  reflect_hash.hpp       # Main header (policies, SIMD, reflection)

analysis/
  hash_quality.hpp       # Quality analysis framework
  run_analysis.cpp       # Quality test runner
  QUALITY_TESTS.md       # Test documentation

benchmarks/
  benchmark_hash.cpp     # Performance benchmarks
  policy_comparison.cpp  # Policy comparison tool

tests/
  test_reflect_hash.cpp  # Unit tests

examples/
  basic_usage.cpp        # Simple usage examples
  custom_algorithm.cpp   # Custom type examples
```

## Dependencies

- **Compiler**: Clang with P2996 reflection (Bloomberg's clang-p2996)
- **Standard**: C++26 (`-std=c++2c -freflection -freflection-latest`)
- **Runtime**: Header-only, no dependencies

## References

1. [P2996: Reflection for C++26](https://wg21.link/p2996)
2. [Folly Hash](https://github.com/facebook/folly/blob/main/folly/hash/Hash.h)
3. [wyhash](https://github.com/wangyi-fudan/wyhash)
4. [xxHash](https://github.com/Cyan4973/xxHash)
5. [MurmurHash](https://github.com/aappleby/smhasher)
6. [simdjson](https://github.com/simdjson/simdjson) - SIMD architecture inspiration
7. [Webster & Tavares (1985): SAC/BIC](https://link.springer.com/chapter/10.1007/3-540-39799-X_41)
