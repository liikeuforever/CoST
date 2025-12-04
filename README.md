# CoST: Cost-aware Trajectory Compression

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

## Citation

```bibtex
todo
```

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file.

## Contact

- **Authors**: [Author Names]
- **Email**: [contact@email.com]
- **Repository**: [Project URL]

---

**Note**: This is an academic research implementation. For production use, additional testing is recommended.
