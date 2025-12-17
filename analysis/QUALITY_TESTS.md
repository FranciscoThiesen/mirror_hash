# Hash Quality Analysis Tests

This document explains the statistical tests used to evaluate hash function quality, their theoretical foundations, and what makes a hash function "good" for use in hash tables.

## Why Quality Matters

A hash function for hash tables must satisfy different properties than cryptographic hashes. We don't need collision resistance against adversaries - we need **uniform distribution** and **good avalanche behavior** to minimize clustering and ensure O(1) average-case performance.

Poor hash functions cause:
- Clustering in hash tables (degraded O(n) performance)
- Increased collision rates
- Predictable patterns that adversaries can exploit (HashDoS attacks)

## Test Suite Overview

| Test | Origin | What It Measures | Pass Criteria |
|------|--------|------------------|---------------|
| Avalanche | Webster & Tavares, 1985 | Bit diffusion | Bias < 0.02 |
| Bit Independence | Webster & Tavares, 1985 | Output bit correlation | Max |r| < 0.1 |
| Chi-Squared | Pearson, 1900 / Knuth TAOCP | Distribution uniformity | p > 0.001 |
| Collision | Birthday Paradox | Hash space utilization | < 10x expected |
| Differential | Biham & Shamir, 1990 | Similar input handling | Bias < 0.05 |
| Permutation | SMHasher | Sparse key behavior | Collision < 0.1% |

---

## 1. Avalanche Analysis

### Origin
**Webster & Tavares (1985)**: "On the Design of S-Boxes" - introduced the Strict Avalanche Criterion (SAC) for evaluating cryptographic components.

### Reference
> A. F. Webster and S. E. Tavares, "On the Design of S-Boxes," Advances in Cryptology — CRYPTO '85, LNCS 218, pp. 523-534, Springer, 1986.

### What It Tests
The **Strict Avalanche Criterion (SAC)** states: when any single input bit is flipped, each output bit should change with probability exactly 0.5.

For a 64-bit hash:
- Flip 1 input bit
- Observe how many output bits change
- Perfect avalanche: 32 bits change on average (50%)

### Why It Matters
Good avalanche ensures that similar inputs produce dissimilar outputs. Without it:
- Sequential keys (0, 1, 2, 3...) may hash to adjacent values
- Small input changes cause small output changes
- Hash table buckets become unevenly loaded

### Our Implementation
```cpp
for each input:
    base_hash = hash(input)
    for each bit position:
        flipped_input = input XOR (1 << bit)
        flipped_hash = hash(flipped_input)
        bits_changed = popcount(base_hash XOR flipped_hash)
        avalanche_ratio = bits_changed / 64
```

### Pass Criteria
- Mean avalanche ratio: 0.50 ± 0.02
- Per-bit flip probability: 0.50 ± 0.02 for each output bit

### Results from Our Analysis
| Hash Function | Avalanche Bias | Result |
|--------------|----------------|--------|
| Boost hash_combine | 0.4756 | **FAIL** |
| Folly hash_128_to_64 | 0.0003 | PASS |
| wyhash | 0.0000 | PASS |
| MurmurHash3 finalizer | 0.0000 | PASS |

---

## 2. Bit Independence Criterion (BIC)

### Origin
Also from **Webster & Tavares (1985)**, the BIC extends SAC to consider relationships between output bits.

### What It Tests
When an input bit is flipped, the resulting changes in any two output bits should be **statistically independent**. We measure the Pearson correlation coefficient between all pairs of output bit changes.

### Why It Matters
Correlated output bits reduce the effective entropy of the hash. If bits 0 and 1 always flip together, you effectively have a 63-bit hash, not 64-bit.

### Our Implementation
```cpp
for each input bit flip:
    record which output bits changed

for each pair of output bits (i, j):
    compute correlation: r = (P(both flip) - P(i flips)*P(j flips)) /
                            sqrt(P(i)*(1-P(i))*P(j)*(1-P(j)))
```

### Pass Criteria
- Mean |correlation| < 0.02
- Max |correlation| < 0.10
- No highly correlated pairs

### Results
| Hash Function | Mean |Corr| | Max |Corr| | Correlated Pairs |
|--------------|---------|---------|------------------|
| Boost hash_combine | 0.0363 | 0.7015 | 89 |
| Folly hash_128_to_64 | 0.0006 | 0.0168 | 0 |

---

## 3. Chi-Squared Distribution Test

### Origin
**Karl Pearson (1900)**: Introduced the chi-squared test for goodness of fit. Applied to hash functions as described in **Knuth, The Art of Computer Programming, Vol. 3**.

### Reference
> D. E. Knuth, "The Art of Computer Programming, Volume 3: Sorting and Searching," Section 6.4, Addison-Wesley, 1998.

### What It Tests
Hash values should be **uniformly distributed** across all possible buckets. We hash N random values into B buckets and measure deviation from the expected uniform distribution.

### The Math
For N items in B buckets:
- Expected count per bucket: E = N/B
- Chi-squared statistic: χ² = Σ (observed - E)² / E
- Degrees of freedom: df = B - 1

### Why It Matters
Non-uniform distribution causes some buckets to be overloaded while others are empty, destroying O(1) performance guarantees.

### Our Implementation
```cpp
buckets[65536] = {0}
for i in 1..1000000:
    h = hash(random_input)
    buckets[h % 65536]++

chi_squared = sum((observed - expected)^2 / expected)
```

### Pass Criteria
- Chi-squared should be close to degrees of freedom
- p-value > 0.001 (not suspiciously uniform or clustered)
- Variance ratio (actual/expected) between 0.8 and 1.2

---

## 4. Collision Analysis

### Origin
Based on the **Birthday Paradox** from probability theory, formalized by von Mises (1939).

### What It Tests
For n random values from a hash space of size 2^b, the expected number of collisions is:

```
E[collisions] ≈ n² / (2 × 2^b)
```

For 10 million 64-bit hashes: expected ≈ 0.003 collisions.

### Why It Matters
Excessive collisions indicate the hash function isn't utilizing the full output space, possibly due to:
- Weak mixing
- Output bits that are always 0 or 1
- Systematic bias

### Our Implementation
```cpp
seen = hash_set()
collisions = 0
for i in 1..10000000:
    h = hash(random_input)
    if h in seen:
        collisions++
    seen.insert(h)
```

### Pass Criteria
- Actual collisions < 10× expected
- For 64-bit hash with 10M inputs, expect ~0 collisions

---

## 5. Differential Analysis

### Origin
**Biham & Shamir (1990)**: Developed differential cryptanalysis for block ciphers. The concept of testing how input differences propagate to output differences is fundamental.

### Reference
> E. Biham and A. Shamir, "Differential Cryptanalysis of DES-like Cryptosystems," Journal of Cryptology, vol. 4, no. 1, pp. 3-72, 1991.

### What It Tests

1. **Sequential inputs**: h(0), h(1), h(2)... should not produce sequential or patterned outputs
2. **Low Hamming distance**: Inputs differing by 1-2 bits should have uncorrelated outputs
3. **Bit position independence**: Changes in high vs low bits should have equal effect

### Why It Matters
Many real-world use cases involve related keys:
- Database IDs: 1, 2, 3, 4...
- Timestamps: consecutive seconds
- IP addresses: 192.168.1.1, 192.168.1.2...

A hash that clusters these destroys performance.

### Our Implementation
```cpp
// Sequential test
prev = hash(0)
for i in 1..100000:
    curr = hash(i)
    avalanche = popcount(prev XOR curr) / 64
    prev = curr

// Hamming-1 test
for each input:
    base = hash(input)
    for each bit:
        h = hash(input XOR (1 << bit))
        measure avalanche
```

### Pass Criteria
- Sequential avalanche: 0.50 ± 0.05
- Hamming-1 avalanche: 0.50 ± 0.05
- High/low bit avalanche should be equal

---

## 6. Permutation (Sparse Key) Test

### Origin
**SMHasher** by Austin Appleby (creator of MurmurHash) - the industry-standard hash testing suite.

### Reference
> https://github.com/aappleby/smhasher

### What It Tests
Hash behavior on **sparse inputs** - values with only a few bits set:
- Single bit: 1, 2, 4, 8, 16...
- Two bits: 3, 5, 6, 9, 10...
- Three bits: 7, 11, 13...

### Why It Matters
Many weak hashes produce collisions on sparse inputs because they don't properly mix low-entropy data. This is especially problematic for:
- Enum values (often powers of 2)
- Flags and bitmasks
- Small integers

### Our Implementation
```cpp
hashes = set()
// Single bit inputs
for i in 0..63:
    hashes.insert(hash(1 << i))

// Two bit inputs
for i in 0..63:
    for j in i+1..63:
        hashes.insert(hash((1 << i) | (1 << j)))

// Check for collisions
collision_rate = 1 - unique_hashes / total_inputs
```

### Pass Criteria
- Collision rate < 0.1%
- Two-bit avalanche ≈ 0.50

---

## SMHasher and Industry Standards

Our test suite is inspired by **SMHasher**, which includes:
- Avalanche tests
- Bit Independence tests
- Differential tests
- Keyset tests (sparse, cyclic, window)
- Speed tests

SMHasher is used to validate:
- MurmurHash, CityHash, xxHash
- Abseil's hash functions
- Rust's default hasher

### What We Don't Test

Our suite omits some SMHasher tests that are less relevant for non-cryptographic hashing:
- **Cryptographic security** - We're not resisting adversaries
- **Seed independence** - We use fixed seeds
- **Key length sensitivity** - We hash fixed structures

---

## Why We Chose Folly's hash_128_to_64

After analyzing multiple candidates:

| Function | Quality | Speed | Notes |
|----------|---------|-------|-------|
| Boost hash_combine | 2/6 | Fastest | Poor quality - don't use |
| FNV-1a | 2/6 | Fast | Poor avalanche |
| Abseil mix | 4/6 | Medium | BIC failures |
| **Folly hash_128_to_64** | **5/6** | **Fast** | **Best balance** |
| wyhash | 5/6 | Fastest | Requires 128-bit multiply |
| MurmurHash3 fmix64 | 5/6 | Fast | Slightly slower |

**Folly was chosen because:**

1. **Proven in production** - Used in Meta's F14 hash tables serving billions of requests
2. **Excellent quality** - 5/6 tests, near-perfect avalanche (bias 0.0003)
3. **Portable** - No 128-bit multiplication (works on all platforms)
4. **Simple** - Only 3 multiplications, easy to understand and verify
5. **Fast** - Competitive with the fastest options

The algorithm originated from CityHash and was refined for F14:

```cpp
constexpr uint64_t hash_128_to_64(uint64_t upper, uint64_t lower) {
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t a = (lower ^ upper) * kMul;
    a ^= (a >> 47);
    uint64_t b = (upper ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;
    return b;
}
```

---

## Running the Analysis

```bash
# Build
cd build && cmake .. && make hash_quality_analysis

# Run
./hash_quality_analysis
```

Output includes:
- Individual test results for each hash function
- Comparison summary table
- Key metrics for easy comparison

---

## References

1. Webster, A. F., & Tavares, S. E. (1985). On the Design of S-Boxes. CRYPTO '85.
2. Knuth, D. E. (1998). The Art of Computer Programming, Vol. 3. Addison-Wesley.
3. Appleby, A. (2011). SMHasher. https://github.com/aappleby/smhasher
4. Facebook/Meta. Folly Hash. https://github.com/facebook/folly/blob/main/folly/hash/Hash.h
5. Wang, Y. (2019). wyhash. https://github.com/wangyi-fudan/wyhash
6. Biham, E., & Shamir, A. (1991). Differential Cryptanalysis. Journal of Cryptology.
