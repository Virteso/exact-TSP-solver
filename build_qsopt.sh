#!/bin/bash
# Build script for qsopt shared library wrapper
# Usage: ./build_qsopt.sh

echo "Building qsopt wrapper library..."

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Detected Linux"
    LIB_NAME="libqsopt_wrapper.so"
    gcc -shared -fPIC -o "$LIB_NAME" qsopt_wrapper.c qsopt.a -lm 2>&1
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected macOS"
    LIB_NAME="libqsopt_wrapper.dylib"
    gcc -shared -fPIC -o "$LIB_NAME" qsopt_wrapper.c qsopt.a -lm 2>&1
else
    echo "Unknown OS: $OSTYPE"
    exit 1
fi

if [ -f "$LIB_NAME" ]; then
    echo "  Successfully built $LIB_NAME"
    echo "  Set QSOPT_LIB=$PWD/$LIB_NAME to use it"
    exit 0
else
    echo "  Failed to build $LIB_NAME"
    exit 1
fi
