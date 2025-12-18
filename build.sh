#!/bin/bash
set -e

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"

echo "=== Building mirror_hash (${BUILD_TYPE}) ==="

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    ..

ninja

echo ""
echo "=== Build Complete ==="
echo "Binaries in: ${BUILD_DIR}/"
echo ""
echo "Run tests:      ./build/test_mirror_hash"
echo "Run benchmarks: ./build/official_comparison"
echo "Run examples:   ./build/example_basic"
