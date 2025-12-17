#!/bin/bash
set -e

BUILD_TYPE="${1:-Release}"
BUILD_DIR="build"

echo "=== Building reflect_hash (${BUILD_TYPE}) ==="

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
echo "Run tests:      ./build/test_reflect_hash"
echo "Run benchmarks: ./build/benchmark_hash"
echo "Run examples:   ./build/example_basic"
