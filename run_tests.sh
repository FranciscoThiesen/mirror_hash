#!/bin/bash
set -e

echo "=== Running reflect_hash Tests ==="
echo ""

if [ ! -f "build/test_reflect_hash" ]; then
    echo "Tests not built. Running build first..."
    ./build.sh
fi

./build/test_reflect_hash

echo ""
echo "=== Running Benchmarks ==="
echo ""

./build/benchmark_hash
