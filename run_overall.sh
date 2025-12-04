#!/bin/bash

echo "======================================"
echo "Running Overall Comparison Experiment"
echo "======================================"
echo ""

cd "$(dirname "$0")"

# Build the experiment
echo "Building comprehensive_perf_test_v3..."
cd experiments/overall
chmod +x build.sh
./build.sh

if [ $? -eq 0 ] && [ -f "overall_comparison" ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Running experiment (this will take several minutes)..."
    ./overall_comparison
    
    # Move results to main CoST directory
    if ls comprehensive_perf_results_*.csv 1> /dev/null 2>&1; then
        mv comprehensive_perf_results_*.csv ../../
        echo ""
        echo "✓ Experiment completed!"
        echo "Results saved to: CoST/comprehensive_perf_results_*.csv"
    fi
else
    echo ""
    echo "✗ Build failed!"
    exit 1
fi
