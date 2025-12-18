#!/bin/bash
set -e

echo "=== Running mirror_hash Tests ==="
echo ""

if [ ! -f "build/test_mirror_hash" ]; then
    echo "Tests not built. Running build first..."
    ./build.sh
fi

./build/test_mirror_hash

echo ""
echo "=== Running Benchmarks ==="
echo ""

./build/official_comparison
