#!/bin/bash

# Build script for C++ TSP solver
cd "$(dirname "$0")"

# Create build directory
mkdir -p build
cd build

# Configure and build
cmake -DCMAKE_BUILD_TYPE=Release ..
make

if [ $? -eq 0 ]; then
    echo "Build successful! Executable: ./build/test_tsp"
else
    echo "Build failed!"
    exit 1
fi
