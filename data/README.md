# GPS Trajectory Datasets

This directory contains the GPS trajectory datasets used in the experiments.

## Datasets

### 1. Geolife
- **File**: `Geolife_100k_with_id.csv`
- **Size**: 100,000 GPS points
- **Source**: Microsoft Research Asia
- **Description**: User mobility trajectories collected from 182 users in Beijing and worldwide
- **Sampling interval**: 1-5 seconds or 5-10 meters
- **Citation**: [Geolife GPS Trajectory Dataset](https://www.microsoft.com/en-us/research/publication/geolife-gps-trajectory-dataset-user-guide/)

### 2. Trajectory (T-Drive)
- **File**: `Trajtory_100k_with_id.csv`
- **Size**: 100,000 GPS points
- **Source**: Microsoft Research Asia
- **Description**: Taxi GPS trajectories in Beijing
- **Sampling interval**: ~177 seconds (average)
- **Citation**: [T-Drive: Driving Directions Based on Taxi Trajectories](https://dl.acm.org/doi/10.1145/1869790.1869807)

### 3. WX-Taxi
- **File**: `WX_taxi_100k_with_id.csv`
- **Size**: 100,000 GPS points
- **Source**: Wuxi City Taxi GPS Data
- **Description**: Taxi trajectories in Wuxi, a medium-sized city in eastern China
- **Sampling interval**: ~30 seconds

## Downsampled Versions

For sampling interval sensitivity experiments, each dataset has downsampled versions:
- `*_downsample_5x.csv`: 1/5 of original points
- `*_downsample_10x.csv`: 1/10 of original points
- `*_downsample_20x.csv`: 1/20 of original points
- `*_downsample_40x.csv`: 1/40 of original points

## Data Format

All datasets follow the same CSV format:

```csv
longitude,latitude,timestamp
116.334255,40.027400,2008-10-23 05:53:05
116.334255,40.027400,2008-10-23 05:53:06
...
```

### Fields

- **longitude**: Longitude in degrees (WGS84 coordinate system)
  - Range: -180° to +180°
  - Precision: 6 decimal places (~0.1 meter accuracy)

- **latitude**: Latitude in degrees (WGS84 coordinate system)
  - Range: -90° to +90°
  - Precision: 6 decimal places (~0.1 meter accuracy)

- **timestamp**: Time in format "YYYY-MM-DD HH:MM:SS"
  - Timezone: Original dataset timezone
  - Converted to Unix timestamp (seconds) by the algorithm

## Usage

The datasets are used by the experiments:

1. **Parameter Sensitivity** (`experiments/sensitivity/`):
   - Uses Trajectory dataset with different downsampling rates
   - Tests error thresholds, sampling intervals, and window sizes

2. **Ablation Studies** (`experiments/ablation/`):
   - Uses all three datasets (original + downsampled versions)
   - Compares CoST with baseline algorithms

3. **Overall Comparison** (`experiments/overall/`):
   - Uses all three original datasets
   - Full comparison with 15 baseline algorithms

## Notes

- **Academic Use Only**: These datasets are for academic research purposes only
- **Citation Required**: Please cite the original papers when using these datasets
- **Privacy**: Do not attempt to identify individuals from the trajectories
- **Commercial Use**: Contact original data providers for commercial licensing

## Data Statistics

| Dataset | Avg Sampling Interval | Avg Speed | Characteristics |
|---------|----------------------|-----------|-----------------|
| Geolife | 1-5 seconds | 10-30 km/h | Multi-modal transport (walk, car, bus, etc.) |
| Trajectory | 177 seconds | 30-50 km/h | Taxi trajectories in urban roads |
| WX-Taxi | 30 seconds | 25-45 km/h | Taxi trajectories in medium-sized city |

## References

1. **Geolife**:
   ```bibtex
   @inproceedings{zheng2008understanding,
     title={Understanding mobility based on GPS data},
     author={Zheng, Yu and Li, Quannan and Chen, Yukun and Xie, Xing and Ma, Wei-Ying},
     booktitle={UbiComp},
     year={2008}
   }
   ```

2. **T-Drive**:
   ```bibtex
   @inproceedings{yuan2010t,
     title={T-drive: driving directions based on taxi trajectories},
     author={Yuan, Jing and Zheng, Yu and others},
     booktitle={GIS},
     year={2010}
   }
   ```

---

Last updated: December 2024
