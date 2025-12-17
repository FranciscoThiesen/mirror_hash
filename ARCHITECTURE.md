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

## Deep Dive: How mirror_hash Works

This section explains the core algorithms and optimizations that make mirror_hash fast.

### The 128-bit Multiply Mixing Function (wymix)

The foundation of mirror_hash is the `wymix` function from wyhash:

```cpp
static constexpr std::uint64_t wymix(std::uint64_t a, std::uint64_t b) noexcept {
    __uint128_t r = static_cast<__uint128_t>(a) * b;
    return static_cast<std::uint64_t>(r) ^ static_cast<std::uint64_t>(r >> 64);
}
```

**Why this works:**
- Multiplying two 64-bit values produces a 128-bit result
- XORing the high and low 64-bit halves creates excellent bit diffusion
- Each input bit influences many output bits (avalanche effect)
- Single CPU instruction on modern processors (`MUL` on x86, `UMULH`+`MUL` on ARM)

### Size-Specialized Hashing Strategies

#### 1-8 Bytes: Single Read, Single Multiply

```
Input:  [b0 b1 b2 b3 b4 b5 b6 b7]  (padded with zeros if < 8 bytes)
        ↓
     read64() with overlap trick for < 8 bytes
        ↓
     wymix(data ^ wyp0 ^ len, INIT_SEED)
        ↓
Output: 64-bit hash
```

**Key optimization**: Standard wyhash uses `wymix(wymix(wyp0, wyp1), data)` (2 multiplies). We precompute `INIT_SEED = wymix(wyp0, wyp1)` at compile time, saving one multiply per hash.

#### 9-16 Bytes: Two Reads, Single Multiply

```
Input:  [b0 b1 b2 ... b15]
        ↓
     a = read64(0..7)    b = read64(8..15)
        ↓
     wymix(a ^ wyp0 ^ len, b ^ INIT_SEED)
        ↓
Output: 64-bit hash
```

**Key insight**: Standard wyhash finalizes with 2 multiplies:
```cpp
// Standard (2 multiplies):
a ^= wyp1; b ^= seed;
r = a * b;              // multiply 1
a = lo(r); b = hi(r);
return wymix(a ^ wyp0 ^ len, b ^ wyp1);  // multiply 2
```

We discovered that for inputs where both halves contain real data, a single multiply suffices:
```cpp
// Optimized (1 multiply):
return wymix(a ^ wyp0 ^ len, b ^ INIT_SEED);
```

This yields **+15-32% speedup** on 8-16 byte inputs.

#### 17-48 Bytes: Incremental Mixing + Fast Finalize

```
Input:  [chunk0: 16B][chunk1: 16B][tail: 1-16B]
        ↓
     seed = INIT_SEED
     seed = combine16(seed, chunk0)  // if > 16B
     seed = combine16(seed, chunk1)  // if > 32B
        ↓
     finalize_fast(seed, tail_a, tail_b, len)
        ↓
Output: 64-bit hash
```

**Key optimization**: After `combine16`, the seed has been through mixing and has good entropy. This means `finalize_fast()` (1 multiply) provides sufficient avalanche instead of `finalize()` (2 multiplies).

#### 49-96 Bytes: Eager Loading + Pre-loaded Finalization

```cpp
// Load ALL data upfront to maximize memory parallelism
w0 = read64<0>(p);   w1 = read64<8>(p);
w2 = read64<16>(p);  w3 = read64<24>(p);
w4 = read64<32>(p);  w5 = read64<40>(p);
w6 = read64<48>(p);  w7 = read64<56>(p);  // if N > 56
// ... more for larger sizes

// Process in pairs
seed = wymix(w0 ^ wyp1, w1 ^ seed);
seed = wymix(w2 ^ wyp1, w3 ^ seed);
// ...

// Finalize using pre-loaded values (no re-reading!)
return finalize_fast(seed, w6, w7, N);
```

**Bug fix**: Original code loaded `w6, w7` but then re-read the same memory locations in finalize. We now use the pre-loaded values directly.

#### 97-128 Bytes: 3-Way Parallel Mixing

```
seed ──┬── acc1 ←── chunk[0..15]
       ├── acc2 ←── chunk[16..31]
       └── acc3 ←── chunk[32..47]
              ↓ (repeat for more chunks)
       combine all accumulators
              ↓
       finalize_fast(seed, tail)
```

**Why 3-way**: At this size, memory latency starts to dominate. Three parallel accumulators hide latency while fitting in registers.

#### 129-512 Bytes: 4-Way Parallel (64 bytes/iteration)

```
for each 64-byte block:
    acc0 = wymix(read64<0> ^ s0, read64<8> ^ acc0)
    acc1 = wymix(read64<16> ^ s1, read64<24> ^ acc1)
    acc2 = wymix(read64<32> ^ s2, read64<40> ^ acc2)
    acc3 = wymix(read64<48> ^ s3, read64<56> ^ acc3)

final_seed = wymix(acc0 ^ acc2, acc1 ^ acc3)
```

**Why 4-way beats 3-way here**: Processing 64 bytes/iteration (vs 48) reduces loop overhead proportionally.

#### 513-4096+ Bytes: 7-Way Parallel (rapidhash-style)

```
for each 112-byte block:
    seed  = wymix(read64<0>  ^ wyp0, read64<8>  ^ seed)
    see1  = wymix(read64<16> ^ wyp1, read64<24> ^ see1)
    see2  = wymix(read64<32> ^ wyp2, read64<40> ^ see2)
    see3  = wymix(read64<48> ^ wyp3, read64<56> ^ see3)
    see4  = wymix(read64<64> ^ wyp4, read64<72> ^ see4)
    see5  = wymix(read64<80> ^ wyp5, read64<88> ^ see5)
    see6  = wymix(read64<96> ^ wyp6, read64<104> ^ see6)

seed = seed ^ see1 ^ see2 ^ see3 ^ see4 ^ see5 ^ see6
```

**Why 7-way**: This matches rapidhash's strategy. At large sizes, we're memory-bandwidth limited, and 7 parallel accumulators maximize throughput on modern CPUs with deep pipelines.

### The finalize vs finalize_fast Trade-off

**Standard finalize (2 multiplies):**
```cpp
static std::size_t finalize(seed, a, b, len) {
    a ^= wyp1; b ^= seed;
    __uint128_t r = a * b;           // Multiply 1
    a = lo(r); b = hi(r);
    return wymix(a ^ wyp0 ^ len, b ^ wyp1);  // Multiply 2
}
```

**Fast finalize (1 multiply):**
```cpp
static std::size_t finalize_fast(seed, a, b, len) {
    __uint128_t r = (a ^ wyp0 ^ len) * (b ^ wyp1 ^ seed);
    return lo(r) ^ hi(r);            // Single multiply
}
```

**When is finalize_fast safe?**
- When seed has been through prior `wymix` operations
- The entropy from prior mixing means one multiply achieves good avalanche
- We verified this passes all 6 quality tests

**Impact:**
- 17-48B: Saves 1 multiply → +22% on 32B
- 49-96B: Saves 1 multiply → +17% on 64B, 96B
- 97-128B: Saves 1 multiply → +11% on 128B
- Larger sizes: Similar savings, 3-8% improvement

### Memory Access Patterns

**Overlapping reads for non-aligned tails:**
```cpp
// For 20-byte input, read last 16 bytes overlapping:
//   [0..7][8..15][16..19]
//              ↓
//   [0..7][8..15]     ← first 16
//         [4..11][12..19] ← last 16 (overlaps bytes 8-15)
```

This avoids branches for alignment and uses the full 64-bit read width.

**Cache-line awareness:**
- Data is read sequentially, maximizing prefetcher effectiveness
- No random access patterns that would cause cache misses
- `alignas(64)` used in benchmarks to avoid cross-cache-line penalties

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
