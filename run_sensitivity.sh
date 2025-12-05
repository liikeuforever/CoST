#!/bin/bash
################################################################################
# Parameter Sensitivity Experiment Quick Run Script
# Tests error_max, sampling interval, and window_size sensitivity
################################################################################

set -e
cd "$(dirname "$0")"

echo "=========================================="
echo "Parameter Sensitivity Experiment"
echo "=========================================="
echo ""

# Build
echo "Building..."
cd experiments/sensitivity
./build.sh 2>&1 | tail -5

# Run
echo ""
echo "Running experiment..."
echo ""
./sensitivity_test 2>&1 | tee ../../sensitivity_experiment_$(date +%Y%m%d_%H%M%S).log

cd ../..

echo ""
echo "=========================================="
echo "Experiment Complete"
echo "=========================================="
echo ""
echo "Result files:"
ls -lt experiments/sensitivity/sensitivity_results_*.csv 2>/dev/null | head -3


