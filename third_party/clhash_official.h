/*
 * CLHash is a very fast hashing function that uses the
 * carry-less multiplication and SSE instructions.
 *
 * Daniel Lemire, Owen Kaser, Faster 64-bit universal hashing
 * using carry-less multiplications, Journal of Cryptographic Engineering (to appear)
 *
 * Best used on recent x64 processors (Haswell or better).
 *
 * Compile option: if you define BITMIX during compilation, extra work is done to
 * pass smhasher's avalanche test succesfully. Disabled by default.
 **/

#ifndef INCLUDE_CLHASH_H_
#define INCLUDE_CLHASH_H_


#include <stdlib.h>
#include <stdint.h> // life is short, please use a C99-compliant compiler
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <x86intrin.h>

#ifdef __WIN32
#define posix_memalign(p, a, s) (((*(p)) = _aligned_malloc((s), (a))), *(p) ?0 :errno)
#endif

enum {RANDOM_64BITWORDS_NEEDED_FOR_CLHASH=133,RANDOM_BYTES_NEEDED_FOR_CLHASH=133*8};

// computes a << 1
static inline __m128i leftshift1(__m128i a) {
    const int x = 1;
    __m128i u64shift =  _mm_slli_epi64(a,x);
    __m128i topbits =  _mm_slli_si128(_mm_srli_epi64(a,64 - x),sizeof(uint64_t));
    return _mm_or_si128(u64shift, topbits);
}

// computes a << 2
static inline __m128i leftshift2(__m128i a) {
    const int x = 2;
    __m128i u64shift =  _mm_slli_epi64(a,x);
    __m128i topbits =  _mm_slli_si128(_mm_srli_epi64(a,64 - x),sizeof(uint64_t));
    return _mm_or_si128(u64shift, topbits);
}

// lazy modulo with 2^127 + 2 + 1
static inline __m128i lazymod127(__m128i Alow, __m128i Ahigh) {
    __m128i shift1 = leftshift1(Ahigh);
    __m128i shift2 = leftshift2(Ahigh);
    __m128i final =  _mm_xor_si128(_mm_xor_si128(Alow, shift1),shift2);
    return final;
}

// multiplication with lazy reduction
static inline  __m128i mul128by128to128_lazymod127( __m128i A, __m128i B) {
    __m128i Amix1 = _mm_clmulepi64_si128(A,B,0x01);
    __m128i Amix2 = _mm_clmulepi64_si128(A,B,0x10);
    __m128i Alow = _mm_clmulepi64_si128(A,B,0x00);
    __m128i Ahigh = _mm_clmulepi64_si128(A,B,0x11);
    __m128i Amix = _mm_xor_si128(Amix1,Amix2);
    Amix1 = _mm_slli_si128(Amix,8);
    Amix2 = _mm_srli_si128(Amix,8);
    Alow = _mm_xor_si128(Alow,Amix1);
    Ahigh = _mm_xor_si128(Ahigh,Amix2);
    return lazymod127(Alow, Ahigh);
}

// multiply the length and the some key, no modulo
static inline __m128i lazyLengthHash(uint64_t keylength, uint64_t length) {
    const __m128i lengthvector = _mm_set_epi64x(keylength,length);
    const __m128i clprod1  = _mm_clmulepi64_si128( lengthvector, lengthvector, 0x10);
    return clprod1;
}

// modulo reduction to 64-bit value
static inline __m128i precompReduction64_si128( __m128i A) {
    const __m128i C = _mm_cvtsi64_si128((1U<<4)+(1U<<3)+(1U<<1)+(1U<<0));
    __m128i Q2 = _mm_clmulepi64_si128( A, C, 0x01);
    __m128i Q3 = _mm_shuffle_epi8(_mm_setr_epi8(0, 27, 54, 45, 108, 119, 90, 65, (char)216, (char)195, (char)238, (char)245, (char)180, (char)175, (char)130, (char)153),
                                  _mm_srli_si128(Q2,8));
    __m128i Q4 = _mm_xor_si128(Q2,A);
    const __m128i final = _mm_xor_si128(Q3,Q4);
    return final;
}

static inline uint64_t precompReduction64( __m128i A) {
    return _mm_cvtsi128_si64(precompReduction64_si128(A));
}

// hashing the bits in value using the keys
static inline uint64_t simple128to64hashwithlength( const __m128i value, const __m128i key, uint64_t keylength, uint64_t length) {
    const __m128i add =  _mm_xor_si128 (value,key);
    const __m128i clprod1  = _mm_clmulepi64_si128( add, add, 0x10);
    const __m128i total = _mm_xor_si128 (clprod1,lazyLengthHash(keylength, length));
    return precompReduction64(total);
}

enum {CLHASH_DEBUG=0};

static inline __m128i __clmulhalfscalarproductwithoutreduction(const __m128i * randomsource, const uint64_t * string,
        const size_t length) {
    assert(((uintptr_t) randomsource & 15) == 0);
    const uint64_t * const endstring = string + length;
    __m128i acc = _mm_setzero_si128();
    for (; string + 3 < endstring; randomsource += 2, string += 4) {
        const __m128i temp1 = _mm_load_si128( randomsource);
        const __m128i temp2 = _mm_lddqu_si128((__m128i *) string);
        const __m128i add1 = _mm_xor_si128(temp1, temp2);
        const __m128i clprod1 = _mm_clmulepi64_si128(add1, add1, 0x10);
        acc = _mm_xor_si128(clprod1, acc);
        const __m128i temp12 = _mm_load_si128(randomsource + 1);
        const __m128i temp22 = _mm_lddqu_si128((__m128i *) (string + 2));
        const __m128i add12 = _mm_xor_si128(temp12, temp22);
        const __m128i clprod12 = _mm_clmulepi64_si128(add12, add12, 0x10);
        acc = _mm_xor_si128(clprod12, acc);
    }
    return acc;
}

static inline __m128i __clmulhalfscalarproductwithtailwithoutreduction(const __m128i * randomsource,
        const uint64_t * string, const size_t length) {
    assert(((uintptr_t) randomsource & 15) == 0);
    const uint64_t * const endstring = string + length;
    __m128i acc = _mm_setzero_si128();
    for (; string + 3 < endstring; randomsource += 2, string += 4) {
        const __m128i temp1 = _mm_load_si128(randomsource);
        const __m128i temp2 = _mm_lddqu_si128((__m128i *) string);
        const __m128i add1 = _mm_xor_si128(temp1, temp2);
        const __m128i clprod1 = _mm_clmulepi64_si128(add1, add1, 0x10);
        acc = _mm_xor_si128(clprod1, acc);
        const __m128i temp12 = _mm_load_si128(randomsource+1);
        const __m128i temp22 = _mm_lddqu_si128((__m128i *) (string + 2));
        const __m128i add12 = _mm_xor_si128(temp12, temp22);
        const __m128i clprod12 = _mm_clmulepi64_si128(add12, add12, 0x10);
        acc = _mm_xor_si128(clprod12, acc);
    }
    if (string + 1 < endstring) {
        const __m128i temp1 = _mm_load_si128(randomsource);
        const __m128i temp2 = _mm_lddqu_si128((__m128i *) string);
        const __m128i add1 = _mm_xor_si128(temp1, temp2);
        const __m128i clprod1 = _mm_clmulepi64_si128(add1, add1, 0x10);
        acc = _mm_xor_si128(clprod1, acc);
        randomsource += 1;
        string += 2;
    }
    if (string < endstring) {
        const __m128i temp1 = _mm_load_si128(randomsource);
        const __m128i temp2 = _mm_loadl_epi64((__m128i const*)string);
        const __m128i add1 = _mm_xor_si128(temp1, temp2);
        const __m128i clprod1 = _mm_clmulepi64_si128(add1, add1, 0x10);
        acc = _mm_xor_si128(clprod1, acc);
    }
    return acc;
}

static inline __m128i __clmulhalfscalarproductwithtailwithoutreductionWithExtraWord(const __m128i * randomsource,
        const uint64_t * string, const size_t length, const uint64_t extraword) {
    assert(((uintptr_t) randomsource & 15) == 0);
    const uint64_t * const endstring = string + length;
    __m128i acc = _mm_setzero_si128();
    for (; string + 3 < endstring; randomsource += 2, string += 4) {
        const __m128i temp1 = _mm_load_si128(randomsource);
        const __m128i temp2 = _mm_lddqu_si128((__m128i *) string);
        const __m128i add1 = _mm_xor_si128(temp1, temp2);
        const __m128i clprod1 = _mm_clmulepi64_si128(add1, add1, 0x10);
        acc = _mm_xor_si128(clprod1, acc);
        const __m128i temp12 = _mm_load_si128(randomsource+1);
        const __m128i temp22 = _mm_lddqu_si128((__m128i *) (string + 2));
        const __m128i add12 = _mm_xor_si128(temp12, temp22);
        const __m128i clprod12 = _mm_clmulepi64_si128(add12, add12, 0x10);
        acc = _mm_xor_si128(clprod12, acc);
    }
    if (string + 1 < endstring) {
        const __m128i temp1 = _mm_load_si128(randomsource);
        const __m128i temp2 = _mm_lddqu_si128((__m128i *) string);
        const __m128i add1 = _mm_xor_si128(temp1, temp2);
        const __m128i clprod1 = _mm_clmulepi64_si128(add1, add1, 0x10);
        acc = _mm_xor_si128(clprod1, acc);
        randomsource += 1;
        string += 2;
    }
    if (string < endstring) {
        const __m128i temp1 = _mm_load_si128(randomsource);
        const __m128i temp2 = _mm_set_epi64x(extraword,*string);
        const __m128i add1 = _mm_xor_si128(temp1, temp2);
        const __m128i clprod1 = _mm_clmulepi64_si128(add1, add1, 0x10);
        acc = _mm_xor_si128(clprod1, acc);
    } else {
        const __m128i temp1 = _mm_load_si128(randomsource);
        const __m128i temp2 = _mm_loadl_epi64((__m128i const*)&extraword);
        const __m128i add1 = _mm_xor_si128(temp1, temp2);
        const __m128i clprod1 = _mm_clmulepi64_si128(add1, add1, 0x01);
        acc = _mm_xor_si128(clprod1, acc);
    }
    return acc;
}

static inline __m128i __clmulhalfscalarproductOnlyExtraWord(const __m128i * randomsource,
        const uint64_t extraword) {
    const __m128i temp1 = _mm_load_si128(randomsource);
    const __m128i temp2 = _mm_loadl_epi64((__m128i const*)&extraword);
    const __m128i add1 = _mm_xor_si128(temp1, temp2);
    const __m128i clprod1 = _mm_clmulepi64_si128(add1, add1, 0x01);
    return clprod1;
}

// there always remain an incomplete word that has 1,2, 3, 4, 5, 6, 7 used bytes.
static inline uint64_t createLastWord(const size_t lengthbyte, const uint64_t * lastw) {
    const int significantbytes = lengthbyte % sizeof(uint64_t);
    uint64_t lastword = 0;
    memcpy(&lastword,lastw,significantbytes);
    return lastword;
}

static inline uint64_t clhash(const void* random, const char * stringbyte,
                const size_t lengthbyte) {
    assert(sizeof(size_t)<=sizeof(uint64_t));
    assert(((uintptr_t) random & 15) == 0);
    const unsigned int  m = 128;
    const int m128neededperblock = m / 2;
    const __m128i * rs64 = (__m128i *) random;
    __m128i polyvalue =  _mm_load_si128(rs64 + m128neededperblock);
    polyvalue = _mm_and_si128(polyvalue,_mm_setr_epi32(0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x3fffffff));
    const size_t length = lengthbyte / sizeof(uint64_t);
    const size_t lengthinc = (lengthbyte + sizeof(uint64_t) - 1) / sizeof(uint64_t);

    const uint64_t * string = (const uint64_t *)  stringbyte;
    if (m < lengthinc) {
        __m128i  acc =  __clmulhalfscalarproductwithoutreduction(rs64, string,m);
        size_t t = m;
        for (; t +  m <= length; t +=  m) {
            acc =  mul128by128to128_lazymod127(polyvalue,acc);
            const __m128i h1 =  __clmulhalfscalarproductwithoutreduction(rs64, string+t,m);
            acc = _mm_xor_si128(acc,h1);
        }
        const int remain = length - t;

        if (remain != 0) {
            acc = mul128by128to128_lazymod127(polyvalue, acc);
            if (lengthbyte % sizeof(uint64_t) == 0) {
                const __m128i h1 =
                    __clmulhalfscalarproductwithtailwithoutreduction(rs64,
                            string + t, remain);
                acc = _mm_xor_si128(acc, h1);
            } else {
                const uint64_t lastword = createLastWord(lengthbyte,
                                          (string + length));
                const __m128i h1 =
                    __clmulhalfscalarproductwithtailwithoutreductionWithExtraWord(
                        rs64, string + t, remain, lastword);
                acc = _mm_xor_si128(acc, h1);
            }
        } else if (lengthbyte % sizeof(uint64_t) != 0) {
            acc = mul128by128to128_lazymod127(polyvalue, acc);
            const uint64_t lastword = createLastWord(lengthbyte, (string + length));
            const __m128i h1 = __clmulhalfscalarproductOnlyExtraWord( rs64, lastword);
            acc = _mm_xor_si128(acc, h1);
        }

        const __m128i finalkey = _mm_load_si128(rs64 + m128neededperblock + 1);
        const uint64_t keylength = *(const uint64_t *)(rs64 + m128neededperblock + 2);
        return simple128to64hashwithlength(acc,finalkey,keylength, (uint64_t)lengthbyte);
    } else {
        if(lengthbyte % sizeof(uint64_t) == 0) {
            __m128i  acc = __clmulhalfscalarproductwithtailwithoutreduction(rs64, string, length);
            const uint64_t keylength = *(const uint64_t *)(rs64 + m128neededperblock + 2);
            acc = _mm_xor_si128(acc,lazyLengthHash(keylength, (uint64_t)lengthbyte));
            return precompReduction64(acc) ;
        }
        const uint64_t lastword = createLastWord(lengthbyte, (string + length));
        __m128i acc = __clmulhalfscalarproductwithtailwithoutreductionWithExtraWord(
                          rs64, string, length, lastword);
        const uint64_t keylength =  *(const uint64_t *)(rs64 + m128neededperblock + 2);
        acc = _mm_xor_si128(acc,lazyLengthHash(keylength, (uint64_t)lengthbyte));
        return precompReduction64(acc) ;
    }
}

// XorShift128+ RNG for key generation
struct xorshift128plus_key_s {
    uint64_t part1;
    uint64_t part2;
};
typedef struct xorshift128plus_key_s xorshift128plus_key_t;

static inline void xorshift128plus_init(uint64_t key1, uint64_t key2, xorshift128plus_key_t *key) {
    key->part1 = key1;
    key->part2 = key2;
}

static inline uint64_t xorshift128plus(xorshift128plus_key_t * key) {
    uint64_t s1 = key->part1;
    const uint64_t s0 = key->part2;
    key->part1 = s0;
    s1 ^= s1 << 23;
    key->part2 = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return key->part2 + s0;
}

static inline void * get_random_key_for_clhash(uint64_t seed1, uint64_t seed2) {
    xorshift128plus_key_t k;
    xorshift128plus_init(seed1, seed2, &k);
    void * answer;
    if (posix_memalign(&answer, sizeof(__m128i),
                       RANDOM_BYTES_NEEDED_FOR_CLHASH)) {
        return NULL;
    }
    uint64_t * a64 = (uint64_t *) answer;
    for(uint32_t i = 0; i < RANDOM_64BITWORDS_NEEDED_FOR_CLHASH; ++i) {
        a64[i] =  xorshift128plus(&k);
    }
    while((a64[128]==0) && (a64[129]==1)) {
        a64[128] =  xorshift128plus(&k);
        a64[129] =  xorshift128plus(&k);
    }
    return answer;
}

#ifdef __cplusplus
#include <vector>
#include <string>
#include <cstring>

struct clhasher {
    const void *random_data_;
    clhasher(uint64_t seed1=137, uint64_t seed2=777): random_data_(get_random_key_for_clhash(seed1, seed2)) {}
    template<typename T>
    uint64_t operator()(const T *data, const size_t len) const {
        return clhash(random_data_, (const char *)data, len * sizeof(T));
    }
    uint64_t operator()(const char *str) const {return operator()(str, std::strlen(str));}
    template<typename T>
    uint64_t operator()(const T &input) const {
        return operator()((const char *)&input, sizeof(T));
    }
    template<typename T>
    uint64_t operator()(const std::vector<T> &input) const {
        return operator()((const char *)input.data(), sizeof(T) * input.size());
    }
    uint64_t operator()(const std::string &str) const {
        return operator()(str.data(), str.size());
    }
    ~clhasher() {
        std::free((void *)random_data_);
    }
};
#endif // #ifdef __cplusplus

#endif /* INCLUDE_CLHASH_H_ */
