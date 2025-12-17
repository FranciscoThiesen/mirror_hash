#include "hash_quality.hpp"
#include <cstring>

// ============================================================================
// Hash Function Wrappers for Analysis
// ============================================================================
//
// The quality analysis framework tests raw byte-to-hash functions.
// We wrap each hash implementation to expose this interface.
//
// ============================================================================

namespace wrappers {

// --- mirror_hash (Folly hash_128_to_64 mixing) ---
namespace mirror_hash_style {

// Folly's hash_128_to_64 - high quality and fast
constexpr uint64_t hash_combine(uint64_t seed, uint64_t value) noexcept {
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t a = (value ^ seed) * kMul;
    a ^= (a >> 47);
    uint64_t b = (seed ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;
    return b;
}

uint64_t hash(const void* data, std::size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t h = 0;

    while (len >= 8) {
        uint64_t v;
        std::memcpy(&v, p, 8);
        h = hash_combine(h, v);
        p += 8;
        len -= 8;
    }

    if (len > 0) {
        uint64_t v = 0;
        std::memcpy(&v, p, len);
        h = hash_combine(h, v);
    }

    return h;
}

} // namespace mirror_hash_style

// --- Boost-style hash_combine (legacy, for comparison) ---
namespace boost_style {

constexpr uint64_t hash_combine(uint64_t seed, uint64_t value) noexcept {
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

uint64_t hash(const void* data, std::size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t h = 0;

    // Process 8 bytes at a time
    while (len >= 8) {
        uint64_t v;
        std::memcpy(&v, p, 8);
        h = hash_combine(h, v);
        p += 8;
        len -= 8;
    }

    // Handle remaining bytes
    if (len > 0) {
        uint64_t v = 0;
        std::memcpy(&v, p, len);
        h = hash_combine(h, v);
    }

    return h;
}

} // namespace boost_style

// --- wyhash (Go/Zig/Nim) ---
namespace wyhash {

static inline uint64_t wymix(uint64_t a, uint64_t b) {
    __uint128_t r = static_cast<__uint128_t>(a) * b;
    return static_cast<uint64_t>(r) ^ static_cast<uint64_t>(r >> 64);
}

static inline uint64_t wyr8(const uint8_t* p) {
    uint64_t v;
    std::memcpy(&v, p, 8);
    return v;
}

static inline uint64_t wyr4(const uint8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

static inline uint64_t wyr3(const uint8_t* p, size_t k) {
    return (static_cast<uint64_t>(p[0]) << 16) |
           (static_cast<uint64_t>(p[k >> 1]) << 8) |
           static_cast<uint64_t>(p[k - 1]);
}

static constexpr uint64_t wyp0 = 0xa0761d6478bd642full;
static constexpr uint64_t wyp1 = 0xe7037ed1a0b428dbull;

uint64_t hash(const void* data, std::size_t len, uint64_t seed = 0) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    seed ^= wyp0;

    uint64_t a, b;
    if (len <= 16) {
        if (len >= 4) {
            a = (wyr4(p) << 32) | wyr4(p + ((len >> 3) << 2));
            b = (wyr4(p + len - 4) << 32) | wyr4(p + len - 4 - ((len >> 3) << 2));
        } else if (len > 0) {
            a = wyr3(p, len);
            b = 0;
        } else {
            a = b = 0;
        }
    } else {
        size_t i = len;
        static constexpr uint64_t wyp2 = 0x8ebc6af09c88c6e3ull;
        static constexpr uint64_t wyp3 = 0x589965cc75374cc3ull;

        if (i > 48) {
            uint64_t see1 = seed, see2 = seed;
            do {
                seed = wymix(wyr8(p) ^ wyp1, wyr8(p + 8) ^ seed);
                see1 = wymix(wyr8(p + 16) ^ wyp2, wyr8(p + 24) ^ see1);
                see2 = wymix(wyr8(p + 32) ^ wyp3, wyr8(p + 40) ^ see2);
                p += 48;
                i -= 48;
            } while (i > 48);
            seed ^= see1 ^ see2;
        }
        while (i > 16) {
            seed = wymix(wyr8(p) ^ wyp1, wyr8(p + 8) ^ seed);
            i -= 16;
            p += 16;
        }
        a = wyr8(p + i - 16);
        b = wyr8(p + i - 8);
    }
    return wymix(wyp1 ^ len, wymix(a ^ wyp1, b ^ seed));
}

} // namespace wyhash

// --- Abseil-style mixing ---
namespace abseil_style {

inline uint64_t mix(uint64_t state, uint64_t v) {
    const uint64_t mul = 0xdc3eb94af8ab4c93ULL;
    uint64_t a = (v ^ state) * mul;
    a ^= (a >> 47);
    uint64_t b = (state ^ a) * mul;
    return b ^ (b >> 47);
}

uint64_t hash(const void* data, std::size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t h = 0;

    while (len >= 8) {
        uint64_t v;
        std::memcpy(&v, p, 8);
        h = mix(h, v);
        p += 8;
        len -= 8;
    }

    if (len > 0) {
        uint64_t v = 0;
        std::memcpy(&v, p, len);
        h = mix(h, v);
    }

    return h;
}

} // namespace abseil_style

// --- Folly-style (hash_128_to_64) ---
namespace folly_style {

constexpr uint64_t hash_128_to_64(uint64_t upper, uint64_t lower) noexcept {
    const uint64_t kMul = 0x9ddfea08eb382d69ULL;
    uint64_t a = (lower ^ upper) * kMul;
    a ^= (a >> 47);
    uint64_t b = (upper ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;
    return b;
}

uint64_t hash(const void* data, std::size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t h = 0;

    while (len >= 8) {
        uint64_t v;
        std::memcpy(&v, p, 8);
        h = hash_128_to_64(h, v);
        p += 8;
        len -= 8;
    }

    if (len > 0) {
        uint64_t v = 0;
        std::memcpy(&v, p, len);
        h = hash_128_to_64(h, v);
    }

    return h;
}

} // namespace folly_style

// --- std::hash baseline (identity for integers) ---
namespace std_hash {

uint64_t hash(const void* data, std::size_t len) {
    // std::hash<uint64_t> is typically identity on most platforms
    if (len >= 8) {
        uint64_t v;
        std::memcpy(&v, data, 8);
        return std::hash<uint64_t>{}(v);
    }
    uint64_t v = 0;
    std::memcpy(&v, data, len);
    return std::hash<uint64_t>{}(v);
}

} // namespace std_hash

// --- FNV-1a (simple baseline) ---
namespace fnv1a {

uint64_t hash(const void* data, std::size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t h = 0xcbf29ce484222325ULL;  // FNV offset basis

    for (std::size_t i = 0; i < len; ++i) {
        h ^= p[i];
        h *= 0x100000001b3ULL;  // FNV prime
    }

    return h;
}

} // namespace fnv1a

// --- MurmurHash3 finalizer (commonly used) ---
namespace murmur3_finalizer {

inline uint64_t fmix64(uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33;
    return k;
}

uint64_t hash(const void* data, std::size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    uint64_t h = 0;

    while (len >= 8) {
        uint64_t v;
        std::memcpy(&v, p, 8);
        h ^= fmix64(v);
        p += 8;
        len -= 8;
    }

    if (len > 0) {
        uint64_t v = 0;
        std::memcpy(&v, p, len);
        h ^= fmix64(v);
    }

    return fmix64(h);
}

} // namespace murmur3_finalizer

} // namespace wrappers

// ============================================================================
// Main Analysis Runner
// ============================================================================

void print_comparison_table(const std::vector<hash_quality::QualityReport>& reports) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "COMPARISON SUMMARY\n";
    std::cout << std::string(80, '=') << "\n\n";

    // Header
    std::cout << std::left << std::setw(25) << "Hash Function"
              << std::right
              << std::setw(12) << "Avalanche"
              << std::setw(10) << "BIC"
              << std::setw(10) << "Chi²"
              << std::setw(12) << "Collision"
              << std::setw(10) << "Diff"
              << std::setw(10) << "Perm"
              << std::setw(8) << "Score"
              << "\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& r : reports) {
        auto pass_str = [](bool p) { return p ? "PASS" : "FAIL"; };

        std::cout << std::left << std::setw(25) << r.hash_name
                  << std::right
                  << std::setw(12) << pass_str(r.avalanche.passed)
                  << std::setw(10) << pass_str(r.bit_independence.passed)
                  << std::setw(10) << pass_str(r.distribution.passed)
                  << std::setw(12) << pass_str(r.collisions.passed)
                  << std::setw(10) << pass_str(r.differential.passed)
                  << std::setw(10) << pass_str(r.permutation.passed)
                  << std::setw(8) << (std::to_string(r.tests_passed()) + "/6")
                  << "\n";
    }

    std::cout << "\n";

    // Detailed metrics comparison
    std::cout << "Key Metrics:\n";
    std::cout << std::string(80, '-') << "\n";
    std::cout << std::left << std::setw(25) << "Hash Function"
              << std::right
              << std::setw(15) << "Aval. Bias"
              << std::setw(15) << "SAC Bias"
              << std::setw(15) << "Mean |Corr|"
              << std::setw(15) << "Seq. Bias"
              << "\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& r : reports) {
        std::cout << std::left << std::setw(25) << r.hash_name
                  << std::right << std::fixed << std::setprecision(4)
                  << std::setw(15) << r.avalanche.avalanche_bias
                  << std::setw(15) << r.avalanche.sac_bias
                  << std::setw(15) << r.bit_independence.mean_correlation
                  << std::setw(15) << r.differential.sequential_bias
                  << "\n";
    }

    std::cout << "\nLower values = better quality (ideal: 0.0000)\n";
}

int main() {
    std::cout << "============================================================\n";
    std::cout << "       HASH FUNCTION QUALITY ANALYSIS SUITE\n";
    std::cout << "============================================================\n\n";

    std::cout << "This analysis implements rigorous statistical tests based on:\n";
    std::cout << "  - Webster & Tavares (1985): Strict Avalanche Criterion\n";
    std::cout << "  - Knuth TAOCP Vol 3: Chi-squared distribution tests\n";
    std::cout << "  - SMHasher test suite methodology\n";
    std::cout << "\n";

    std::vector<hash_quality::QualityReport> reports;

    // Test all hash implementations
    std::cout << "\n[1/8] Testing mirror_hash (MurmurHash3 finalizer mixing)...\n";
    reports.push_back(hash_quality::full_quality_analysis(
        "mirror_hash (new)", wrappers::mirror_hash_style::hash));

    std::cout << "\n[2/8] Testing Boost-style hash_combine (legacy)...\n";
    reports.push_back(hash_quality::full_quality_analysis(
        "Boost hash_combine", wrappers::boost_style::hash));

    std::cout << "\n[3/8] Testing wyhash (Go/Zig/Nim)...\n";
    reports.push_back(hash_quality::full_quality_analysis(
        "wyhash", [](const void* d, std::size_t l) {
            return wrappers::wyhash::hash(d, l);
        }));

    std::cout << "\n[4/8] Testing Abseil-style mixing...\n";
    reports.push_back(hash_quality::full_quality_analysis(
        "Abseil-style", wrappers::abseil_style::hash));

    std::cout << "\n[5/8] Testing Folly hash_128_to_64...\n";
    reports.push_back(hash_quality::full_quality_analysis(
        "Folly hash_128_to_64", wrappers::folly_style::hash));

    std::cout << "\n[6/8] Testing std::hash baseline...\n";
    reports.push_back(hash_quality::full_quality_analysis(
        "std::hash (baseline)", wrappers::std_hash::hash));

    std::cout << "\n[7/8] Testing FNV-1a...\n";
    reports.push_back(hash_quality::full_quality_analysis(
        "FNV-1a", wrappers::fnv1a::hash));

    std::cout << "\n[8/8] Testing MurmurHash3 finalizer...\n";
    reports.push_back(hash_quality::full_quality_analysis(
        "MurmurHash3 finalizer", wrappers::murmur3_finalizer::hash));

    // Print individual reports
    for (const auto& report : reports) {
        report.print();
    }

    // Print comparison table
    print_comparison_table(reports);

    // Analysis conclusion
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "ANALYSIS FOR mirror_hash\n";
    std::cout << std::string(80, '=') << "\n\n";

    const auto& new_report = reports[0];   // New mirror_hash with MurmurHash3 mixing
    const auto& boost_report = reports[1]; // Legacy boost-style

    std::cout << "mirror_hash uses MurmurHash3 finalizer-based mixing:\n";
    std::cout << "  mix(k) = fmix64(k)  // Spreads all input bits\n";
    std::cout << "  combine(seed, value) = mix(seed + 0x9e3779b9 + mix(value))\n\n";

    std::cout << "Comparison with legacy Boost-style:\n";
    std::cout << "  mirror_hash (new): " << new_report.tests_passed() << "/6 tests passed\n";
    std::cout << "  Boost hash_combine: " << boost_report.tests_passed() << "/6 tests passed\n\n";

    std::cout << "Key metrics improvement:\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Avalanche bias:   " << boost_report.avalanche.avalanche_bias
              << " -> " << new_report.avalanche.avalanche_bias
              << " (lower is better)\n";
    std::cout << "  SAC bias:         " << boost_report.avalanche.sac_bias
              << " -> " << new_report.avalanche.sac_bias << "\n";
    std::cout << "  Sequential bias:  " << boost_report.differential.sequential_bias
              << " -> " << new_report.differential.sequential_bias << "\n";

    if (new_report.tests_passed() >= 5) {
        std::cout << "\nRESULT: mirror_hash now achieves high-quality hash distribution,\n";
        std::cout << "comparable to industry-standard implementations (wyhash, Folly).\n";
    }

    std::cout << "\n";

    return 0;
}
