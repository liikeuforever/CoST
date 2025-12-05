# CoST: Efficient and Error-Bounded Compression of Streaming Trajectories

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Official implementation of **CoST** (Cost-aware Simple Trajectory Compression), an efficient lossy compression algorithm for GPS trajectory data.

## Key Features

- **Cost-based Predictor Selection**: Dynamic evaluation of three predictors (LDR/CP/ZP)
- **Intelligent Mode Switching**: Adaptive switching between Multi-Predictor and LDR-Only modes
- **Error Guarantee**: Strict Euclidean distance bounds within user-specified thresholds
- **High Performance**: 0.054-0.062 compression ratio, 0.35-0.42 μs/point compression speed

## Quick Start

### Requirements

- C++17 compiler (GCC 9+ or Clang 10+)
- CMake 3.10+ (optional, for comprehensive experiments)

### Basic Usage

```cpp
#include "algorithm/cost_compressor.h"

// Create compressor
CoSTCompressor compressor(
    100000,     // block_size
    1e-5,       // epsilon (error threshold in degrees)
    96,         // evaluation_window
    false       // use_time_window
);

// Add GPS points
for (const auto& point : trajectory) {
    compressor.AddGpsPoint(CoSTCompressor::GpsPoint(
        point.longitude, point.latitude, point.timestamp
    ));
}

// Finalize and get compressed data
compressor.Close();
Array<uint8_t> compressed = compressor.GetCompressedData();

// Decompress
CoSTDecompressor decompressor(compressed.begin(), compressed.length());
CoSTDecompressor::GpsPoint point;
while (decompressor.ReadNextPoint(point)) {
    // Process decompressed point
}
```

## Project Structure

```
CoST/
├── README.md                          # This file
├── LICENSE                            # MIT License
├── algorithm/                         # Core algorithm
│   ├── cost_compressor.h
│   └── cost_compressor.cc
├── experiments/                       # Reproducibility scripts
│   ├── sensitivity/                   # Parameter sensitivity analysis
│   ├── ablation/                      # Ablation studies
│   └── overall/                       # Comprehensive comparison
├── baselines/                         # 20 baseline algorithms
├── data/                              # GPS datasets (3 main + downsampled)
└── utils/                             # Utility classes
```

## Reproducing Experiments

### Quick Run (from CoST root)

```bash
# Parameter Sensitivity (~5-10 minutes)
./run_sensitivity.sh

# Ablation Study (~15-30 minutes)
./run_ablation.sh

# Overall Comparison (~15-30 minutes, requires main Serf project)
./run_overall.sh
```

### Manual Run

#### Parameter Sensitivity (Section 5.2)

```bash
cd experiments/sensitivity
./build.sh
./sensitivity_test
```

Tests error threshold, sampling interval, and window size sensitivity.

#### Ablation Studies (Section 5.3)

```bash
cd experiments/ablation
./build.sh
./ablation_test all
```

Compares CoST with TrajCompress-SP, Serf-QT, and single-predictor variants.

#### Comprehensive Comparison (Section 5.4)

```bash
./CoST/run_overall.sh
```

Compares CoST with 20 baseline algorithms across 5 categories.

**Note**: This experiment requires the full Serf project with CMake and takes ~15-30 minutes to run.

## Algorithm Overview

CoST employs three predictors:
- **LP** (Linear DPredictor): For constant velocity motion
- **CP** (Curve Predictor): For acceleration and turning
- **ZP** (Zero Predictor): For stationary motion

The algorithm dynamically selects the predictor with minimum encoding cost (Huffman flag + quantized error) and intelligently switches between Multi-Predictor and LDR-Only modes based on sliding window cost evaluation.

## Datasets

Three real-world GPS trajectory datasets are provided in `data/`:

- **Geolife**: 100k points, multi-modal transport
- **Trajectory**: 100k points, Beijing taxi data
- **WX-Taxi**: 100k points, Wuxi taxi data

Each includes downsampled versions (5x, 10x, 20x, 40x) for sensitivity analysis.

## Floating-Point Compression Baselines

The `baselines/` directory includes implementations of state-of-the-art floating-point compression algorithms used for comparison with CoST. These algorithms are categorized into streaming and batch processing methods.

### Streaming Algorithms

These algorithms process data incrementally and are suitable for real-time compression:

1. **Chimp128**
   - Streaming floating-point compressor
   - Uses previous 128 values for prediction
   - Efficient XOR-based encoding
   - Location: `baselines/chimp128/`

2. **Elf** (Error-bounded Lossy Floating-point compression)
   - Error-bounded floating-point compression
   - Supports both double and float precision
   - Uses predictive coding with error control
   - Location: `baselines/elf/`

3. **Serf** (Streaming Error-bounded Floating-point compression)
   - Streaming error-bounded compression framework
   - **Serf-QT**: Quantization-based variant
   - **Serf-XOR**: XOR-based variant
   - Block-based processing with error guarantees
   - Location: `baselines/serf/`

### Batch Processing Algorithms

These algorithms process data in blocks and are optimized for throughput:

4. **SZ2**
   - Scientific data compression library
   - Multi-dimensional data compression
   - Supports various error bounds and compression modes
   - Location: `baselines/sz2/`

5. **Machete**
   - Lorenzo predictor-based compression
   - Multiple encoder options: Huffman, OVLQ, Hybrid
   - Template-based design for flexibility
   - Location: `baselines/machete/`

### Usage in Experiments

These baseline algorithms are integrated into the comprehensive performance comparison experiments (`experiments/overall/`), where they are evaluated alongside CoST on GPS trajectory datasets. The streaming algorithms (Chimp128, Elf, Serf) are particularly relevant as they share similar use cases with CoST for real-time trajectory compression.

## Trajectory Compression Baseline Testing Module

The `baselines/traj-module/` directory contains a comprehensive testing framework for comparing trajectory compression baseline algorithms. This module provides implementations and performance evaluation tools for 5 classical trajectory compression algorithms.

### Purpose

This module is designed for benchmarking and comparing baseline trajectory compression algorithms against CoST. It includes automated testing scripts, performance metrics collection, and result visualization tools.

### Algorithms Included

The `traj-module` implements the following trajectory compression algorithms:

1. **Dead Reckoning (DR)**
   - Dead reckoning-based trajectory compression
   - Uses velocity prediction for trajectory simplification

2. **Douglas-Peucker (DP)**
   - Classic line simplification algorithm
   - Recursive point elimination based on perpendicular distance

3. **OPW-TR** (Opening Window Time Ratio)
   - Time-aware trajectory compression
   - Uses temporal windows for compression decisions

4. **SQUISH-E** (Spatial Quality Simplification)
   - Priority queue-based compression
   - Optimizes for spatial error bounds

5. **VOLTCom** (Vector Trajectory Compression)
   - Vector-based trajectory representation
   - Uses Bézier curves for trajectory approximation

### Module Structure

```
baselines/traj-module/
├── README_TEST.md              # Detailed technical documentation
├── compile_all.sh             # Build script for all algorithms
├── run_tests.py               # Automated testing framework
├── run_quick_test.py          # Quick test (10k points)
├── analyze_results.py         # Result analysis and visualization
├── data_set/                  # Test datasets (3 datasets × 100k points)
├── Dead_Reckoning_Test/       # Dead Reckoning implementation
├── DouglasPeuckerTest/        # Douglas-Peucker implementation
├── OPW-TR-Test/               # OPW-TR implementation
├── SQUISH_E_Test/             # SQUISH-E implementation
└── VOLTComTest/               # VOLTCom implementation
```

### Quick Start

```bash
# Navigate to traj-module directory
cd baselines/traj-module

# Compile all algorithms (macOS/Linux)
./compile_all.sh

# Run quick test (recommended for first-time users)
python3 run_quick_test.py

# Run full test suite
python3 run_tests.py
```

Test results are saved in `test_results/` directory with detailed performance metrics including compression ratio, compression/decompression time, and error statistics.

For more details, see [baselines/traj-module/README_TEST.md](baselines/traj-module/README_TEST.md).

## Citation

If you use CoST in your research, please cite:

```bibtex
@article{cost2024,
  title={CoST: Efficient and Error-Bounded Compression of Streaming Trajectories},
  author={[Authors]},
  journal={[Journal/Conference]},
  year={2024}
}
```

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file.

## Contact

- **Repository**: [https://github.com/liikeuforever/CoST](https://github.com/liikeuforever/CoST)
- **Issues**: Please use [GitHub Issues](https://github.com/liikeuforever/CoST/issues) for bug reports and feature requests

---

**Note**: This is an academic research implementation. For production use, additional testing is recommended.
