# mirror_hash Architecture

This document describes the design decisions, implementation details, and performance analysis of mirror_hash - a configurable, reflection-based hashing library for C++26.

## Overview

mirror_hash leverages C++26 reflection (P2996) to automatically generate hash functions for arbitrary user-defined types. It features:

- **Zero Boilerplate**: Hash any struct without writing custom hash functions
- **Configurable Policies**: Choose from multiple hash algorithms (wyhash, Folly, MurmurHash3, xxHash3)
- **Compile-Time Optimization**: Size-specialized paths for maximum performance
- **High Quality**: All recommended policies pass rigorous statistical tests

## Design Principles

### 1. Policy-Based Design (Zero Runtime Overhead)

Hash algorithms are selected at compile time via template parameters:

```cpp
// Default (wyhash) - no runtime dispatch
auto h1 = mirror_hash::hash(my_struct);

// Explicit policy selection - still no runtime dispatch
auto h2 = mirror_hash::hash<mirror_hash::folly_policy>(my_struct);

// With containers
std::unordered_set<MyType, mirror_hash::hasher<mirror_hash::wyhash_policy>> set;
```

The policy is a compile-time template parameter, so the compiler can inline everything. There is **zero overhead** compared to a hardcoded implementation.

### 2. Compile-Time Size Specialization

When the size of data is known at compile time, mirror_hash uses specialized code paths:

```cpp
template<typename Policy, std::size_t N>
std::size_t hash_bytes_fixed(const void* data);
```

This enables:
- Complete loop unrolling for small sizes
- Optimal read patterns (e.g., 2x64-bit reads for 16 bytes)
- Precomputed constants (INIT_SEED saves one multiply per hash)
- Size-appropriate parallelism (3-way for medium, 7-way for large)

### 3. Concept-Based Type Dispatch

Types are automatically routed to the best hashing strategy:

| Concept | Strategy |
|---------|----------|
| `Arithmetic` | Direct value hashing |
| `ContiguousContainer<Arithmetic>` | Bulk byte hashing |
| `Container` | Per-element hashing |
| `Reflectable` | Compile-time member iteration |

## Hash Policies

### Available Policies

| Policy | Quality | Speed | Notes |
|--------|---------|-------|-------|
| `wyhash_policy` (default) | 6/6 | **Fastest** | Go/Zig/Nim default, 128-bit multiply |
| `folly_policy` | 6/6 | Fast | Meta's F14 hash tables |
| `murmur3_policy` | 6/6 | Medium | Classic, very portable |
| `xxhash3_policy` | 6/6 | Fast | Modern, SIMD-friendly |
| `rapidhash_policy` | 6/6 | Fast | Optimized wyhash variant |
| `fnv1a_policy` | 2/6 | Fast | **NOT RECOMMENDED** - poor avalanche |

### Why wyhash as Default

After quality analysis and benchmarking:

1. **Industry standard** - Default in Go, Zig, Nim programming languages
2. **Excellent quality** - 6/6 tests passed, near-perfect avalanche (0.499-0.501)
3. **Fastest** - 128-bit multiply provides excellent mixing with minimal operations
4. **Battle-tested** - Used in production by major projects worldwide

## Performance Analysis

### Benchmark Against Official Implementations

mirror_hash is benchmarked against official implementations from their respective repositories:

- **wyhash v4.2**: https://github.com/wangyi-fudan/wyhash
- **rapidhash V3**: https://github.com/Nicoshev/rapidhash
- **xxHash 0.8.3**: https://github.com/Cyan4973/xxHash
- **clhash**: https://github.com/lemire/clhash (x86 only, CLMUL-based)

### Results (Docker container, aarch64)

| Size | mirror_hash_fixed | Best Competitor | Improvement |
|------|-------------------|-----------------|-------------|
| 8B | 0.49ns | rapidhash 0.56ns | **+15%** |
| 16B | 0.47ns | rapidhash 0.62ns | **+32%** |
| 32B | 0.76ns | wyhash 0.93ns | **+22%** |
| 64B | 1.37ns | wyhash 1.61ns | **+17%** |
| 96B | 2.18ns | wyhash 2.55ns | **+17%** |
| 128B | 2.65ns | wyhash 2.94ns | **+11%** |
| 256B | 5.84ns | rapidhash 6.28ns | **+8%** |
| 512B | 11.40ns | rapidhash 11.79ns | **+3%** |
| 1024B | 20.63ns | rapidhash 21.61ns | **+5%** |

**Key observations:**
- mirror_hash wins on ALL 9 sizes tested
- Massive improvement on small sizes: +15-32% (8-16B)
- Significant gains on medium sizes: +11-22% (32-128B)
- Consistent 3-8% improvement on large sizes (256-1024B)

### Why mirror_hash is Fast

1. **Single-multiply optimization**: For 1-16 bytes, uses only ONE 128-bit multiply instead of two
2. **Fast finalization**: For sizes > 16B, `finalize_fast()` uses 1 multiply instead of 2 (seed already has good entropy)
3. **Precomputed INIT_SEED**: Saves one 128-bit multiply per hash
4. **Size-specialized paths**: Different strategies for 1-8B, 9-16B, 17-48B, 49-96B, 97-128B, 129-512B, 513-1024B, >1024B
5. **Optimal parallelism**: 3-way for medium sizes, 7-way for large (matches rapidhash)
6. **Compile-time constants**: All branching resolved at compile time

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
wyhash              PASS       PASS   PASS   PASS       PASS   PASS   6/6
folly               PASS       PASS   PASS   PASS       PASS   PASS   6/6
murmur3             PASS       PASS   PASS   PASS       PASS   PASS   6/6
xxhash3             PASS       PASS   PASS   PASS       PASS   PASS   6/6
fnv1a               FAIL       FAIL   PASS   PASS       FAIL   FAIL   2/6
```

## Implementation Details

### Reflection-Based Member Iteration

```cpp
template<typename T>
consteval std::size_t member_count() {
    return std::meta::nonstatic_data_members_of(
        ^^T, std::meta::access_context::unchecked()
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

### wyhash_policy Implementation

```cpp
struct wyhash_policy {
    static constexpr std::uint64_t wyp0 = 0xa0761d6478bd642full;
    static constexpr std::uint64_t wyp1 = 0xe7037ed1a0b428dbull;

    // Precomputed: wymix(wyp0, wyp1) - saves 1 multiply per hash
    static constexpr std::uint64_t INIT_SEED = 0x1ff5c2923a788d2cull;

    static constexpr std::uint64_t wymix(std::uint64_t a, std::uint64_t b) noexcept {
        __uint128_t r = static_cast<__uint128_t>(a) * b;
        return static_cast<std::uint64_t>(r) ^ static_cast<std::uint64_t>(r >> 64);
    }

    static constexpr std::size_t combine(std::size_t seed, std::size_t value) noexcept {
        return wymix(seed ^ wyp0, value ^ wyp1);
    }

    // Fast finalization: 1 multiply instead of 2 (seed already has good entropy)
    // Key insight: After prior mixing, one 128-bit multiply provides sufficient avalanche
    static constexpr std::size_t finalize_fast(std::size_t seed, std::uint64_t a,
                                                std::uint64_t b, std::size_t len) noexcept {
        __uint128_t r = static_cast<__uint128_t>(a ^ wyp0 ^ len) * (b ^ wyp1 ^ seed);
        return static_cast<std::uint64_t>(r) ^ static_cast<std::uint64_t>(r >> 64);
    }
};
```

## File Structure

```
include/mirror_hash/
  mirror_hash.hpp       # Main header (policies, size specialization, reflection)

analysis/
  hash_quality.hpp      # Quality analysis framework
  run_analysis.cpp      # Quality test runner
  QUALITY_TESTS.md      # Test documentation

benchmarks/
  official_comparison.cpp  # Benchmark vs wyhash, rapidhash, xxHash
  policy_comparison.cpp    # Compare different mirror_hash policies
  quality_verification.cpp # Verify hash quality metrics

tests/
  test_mirror_hash.cpp  # Unit tests

examples/
  basic_usage.cpp       # Simple usage examples
  custom_algorithm.cpp  # Custom policy examples
  hashtable_integration.cpp # std::unordered_* integration
```

## Dependencies

- **Compiler**: Clang with P2996 reflection (Bloomberg's clang-p2996)
- **Standard**: C++26 (`-std=c++2c -freflection -freflection-latest`)
- **Runtime**: Header-only, no dependencies

## References

1. [P2996: Reflection for C++26](https://wg21.link/p2996)
2. [wyhash](https://github.com/wangyi-fudan/wyhash)
3. [rapidhash](https://github.com/Nicoshev/rapidhash)
4. [xxHash](https://github.com/Cyan4973/xxHash)
5. [clhash](https://github.com/lemire/clhash)
6. [Folly Hash](https://github.com/facebook/folly/blob/main/folly/hash/Hash.h)
7. [MurmurHash](https://github.com/aappleby/smhasher)
8. [Webster & Tavares (1985): SAC/BIC](https://link.springer.com/chapter/10.1007/3-540-39799-X_41)
