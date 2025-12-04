#!/bin/bash

echo "======================================"
echo "Building Parameter Sensitivity Test"
echo "======================================"

# Navigate to script directory
cd "$(dirname "$0")"

# Check if CoST baselines are available
if [ ! -d "../../baselines/serf" ]; then
    echo "Error: Serf baseline libraries not found in baselines/serf"
    echo "Please ensure CoST project structure is complete."
    exit 1
fi

# Compile
g++ -std=c++17 -O3 \
    sensitivity_test.cc \
    ../../algorithm/cost_compressor.cc \
    ../../baselines/serf/serf_qt_compressor.cc \
    ../../baselines/serf/serf_qt_decompressor.cc \
    ../../utils/elias_delta_codec.cc \
    ../../utils/elias_gamma_codec.cc \
    ../../utils/output_bit_stream.cc \
    ../../utils/input_bit_stream.cc \
    -I../../ \
    -o sensitivity_test

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Usage:"
    echo "  ./sensitivity_test"
    echo ""
    echo "Output: sensitivity_results_YYYYMMDD_HHMMSS.csv"
else
    echo "✗ Build failed!"
    exit 1
fi
