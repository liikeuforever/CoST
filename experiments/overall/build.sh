#!/bin/bash

echo "======================================"
echo "Building Overall Comparison"
echo "======================================"

cd "$(dirname "$0")"

ARCH=$(uname -m)
echo "Architecture: $ARCH"
echo "Note: Testing 17 algorithms (excluding SZ2, Machete, Sprintz)"
echo ""

# All source files
g++ -std=c++17 -O3 -Wno-register \
    comprehensive_perf_test_v3.cc \
    ../../algorithm/cost_compressor.cc \
    ../../utils/output_bit_stream.cc \
    ../../utils/input_bit_stream.cc \
    ../../utils/elias_gamma_codec.cc \
    ../../utils/elias_delta_codec.cc \
    ../../utils/serf_utils_64.cc \
    ../../utils/post_office_solver.cc \
    ../../baselines/serf/serf_qt_compressor.cc \
    ../../baselines/serf/serf_qt_decompressor.cc \
    ../../baselines/serf/serf_xor_compressor.cc \
    ../../baselines/serf/serf_xor_decompressor.cc \
    ../../baselines/serf/net_serf_qt_compressor.cc \
    ../../baselines/serf/net_serf_qt_decompressor.cc \
    ../../baselines/serf/net_serf_xor_compressor.cc \
    ../../baselines/serf/net_serf_xor_decompressor.cc \
    ../../baselines/gorilla/gorilla_compressor.cc \
    ../../baselines/gorilla/gorilla_decompressor.cc \
    ../../baselines/chimp128/chimp_compressor.cc \
    ../../baselines/chimp128/chimp_decompressor.cc \
    ../../baselines/deflate/deflate_compressor.cc \
    ../../baselines/deflate/deflate_decompressor.cc \
    ../../baselines/deflate/*.c \
    ../../baselines/fpc/fpc_compressor.cc \
    ../../baselines/fpc/fpc_decompressor.cc \
    ../../baselines/lz77/fastlz.c \
    ../../baselines/snappy/snappy.cc \
    ../../baselines/snappy/snappy-sinksource.cc \
    ../../baselines/sim_piece/sim_piece.cc \
    ../../baselines/sim_piece/float_encoder.cc \
    ../../baselines/sim_piece/int_encoder.cc \
    ../../baselines/sim_piece/u_int_encoder.cc \
    ../../baselines/sim_piece/variable_byte_encoder.cc \
    ../../baselines/elf/ElfCompressor.cpp \
    ../../baselines/elf/ElfDecompressor.cpp \
    ../../baselines/elf/ElfCompressor32.cpp \
    ../../baselines/elf/ElfDecompressor32.cpp \
    ../../baselines/elf/utils.cc \
    ../../baselines/lz4/lz4_compressor.cc \
    ../../baselines/lz4/lz4_decompressor.cc \
    ../../baselines/lz4/lz4.c \
    ../../baselines/lz4/lz4frame.c \
    ../../baselines/lz4/lz4hc.c \
    ../../baselines/lz4/xxhash.c \
    -I../../ -I../../baselines/zstd/lib \
    ../../baselines/zstd/lib/libzstd.a \
    -o overall_comparison \
    -lm -lpthread -lz 2>&1 | grep -v "warning: treating 'c' input"

COMPILE_STATUS=${PIPESTATUS[0]}

if [ $COMPILE_STATUS -eq 0 ] && [ -f "overall_comparison" ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Usage: ./overall_comparison"
    echo "Algorithms: 17 (CoST + 16 baselines)"
    ls -lh overall_comparison
else
    echo ""
    echo "✗ Build failed"
    exit 1
fi
