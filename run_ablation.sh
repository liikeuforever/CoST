#!/bin/bash
################################################################################
# Ablation Study Quick Run Script
# Tests single-predictor vs multi-predictor vs CoST algorithm
################################################################################

set -e
cd "$(dirname "$0")"

echo "=========================================="
echo "Ablation Study Experiment"
echo "=========================================="
echo ""

# Build
echo "Building..."
cd experiments/ablation
./build.sh 2>&1 | tail -5

# Run
echo ""
echo "Running experiment..."
echo ""
./ablation_test all 2>&1 | tee ../../ablation_experiment_$(date +%Y%m%d_%H%M%S).log

cd ../..

echo ""
echo "=========================================="
echo "Experiment Complete"
echo "=========================================="
echo ""
echo "Result files:"
ls -lt experiments/ablation/compression_results_*.csv experiments/ablation/paper_comparison_table_*.csv 2>/dev/null | head -5


