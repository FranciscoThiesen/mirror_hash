#include <cstdint>
#include <cstddef>
#include "../third_party/smhasher/rapidhash.h"

__attribute__((noinline))
uint64_t test_rapidhash_nano_8bytes(const void* data, uint64_t seed) {
    return rapidhashNano_withSeed(data, 8, seed);
}

__attribute__((noinline))
uint64_t test_rapidhash_nano_16bytes(const void* data, uint64_t seed) {
    return rapidhashNano_withSeed(data, 16, seed);
}

int main() {
    char data[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    volatile uint64_t h = test_rapidhash_nano_8bytes(data, 0);
    h = test_rapidhash_nano_16bytes(data, 0);
    return h & 1;
}
