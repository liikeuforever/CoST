#!/bin/bash

echo "======================================"
echo "Building Ablation Study Experiments"
echo "======================================"

# Navigate to script directory
cd "$(dirname "$0")"

echo "Note: This experiment requires baseline algorithm dependencies."
echo ""

# Check if baseline libraries are available
if [ ! -d "../../baselines/serf" ] || [ ! -d "../../baselines/trajcompress" ]; then
    echo "Error: Baseline libraries not found."
    echo "Please ensure CoST project structure is complete with:"
    echo "  - baselines/serf/"
    echo "  - baselines/trajcompress/"
    exit 1
fi

# Compile
g++ -std=c++17 -O3 \
    ablation_test.cc \
    ../../baselines/trajcompress/trajcompress_sp_compressor.cc \
    ../../baselines/trajcompress/trajcompress_sp_adaptive_compressor.cc \
    ../../algorithm/cost_compressor.cc \
    ../../baselines/serf/serf_qt_compressor.cc \
    ../../baselines/serf/serf_qt_linear_compressor.cc \
    ../../baselines/serf/serf_qt_curve_compressor.cc \
    ../../baselines/serf/serf_qt_decompressor.cc \
    ../../baselines/serf/serf_qt_linear_decompressor.cc \
    ../../baselines/serf/serf_qt_curve_decompressor.cc \
    ../../utils/output_bit_stream.cc \
    ../../utils/input_bit_stream.cc \
    ../../utils/elias_gamma_codec.cc \
    -I../../ \
    -o ablation_test \
    -lm

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Usage:"
    echo "  ./ablation_test all              # Test all datasets"
    echo "  ./ablation_test single <path>    # Test single dataset"
    echo ""
    echo "Output: compression_results_YYYYMMDD_HHMMSS.csv"
    echo "        paper_comparison_table_YYYYMMDD_HHMMSS.csv"
else
    echo "✗ Build failed!"
    exit 1
fi
