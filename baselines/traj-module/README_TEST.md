# 轨迹压缩算法性能测试

## 项目概述

本项目包含5种轨迹压缩算法的实现和性能测试框架：

1. **Dead Reckoning (DR)** - 航位推算算法
2. **Douglas-Peucker (DP)** - 道格拉斯-普克算法
3. **OPW-TR** - 开窗时间比算法
4. **SQUISH-E** - 优先队列压缩算法
5. **VOLTCom** - 向量轨迹压缩算法

## 数据集

测试使用3个真实世界的GPS轨迹数据集（各100k个点）：

- `Geolife_100k_with_id.csv` - Geolife轨迹数据集
- `Trajtory_100k_with_id.csv` - WorldTrace轨迹数据集
- `WX_taxi_100k_with_id.csv` - 武汉出租车轨迹数据集

数据集位于 `data_set/` 目录下。

## 快速开始

### 方法1：使用自动化测试脚本（推荐）

#### macOS/Linux

1. **编译所有算法**

```bash
chmod +x compile_all.sh
./compile_all.sh
```

2. **运行测试**

```bash
python3 run_tests.py
```

测试结果将保存在 `test_results/` 目录下。

#### Windows

在 Windows 上，项目使用 Visual Studio 预编译的可执行文件。直接运行：

```cmd
python run_tests.py
```

### 方法2：手动编译和测试

#### macOS/Linux 手动编译

```bash
# 创建 build 目录
mkdir -p build

# 编译单个算法（以 Dead_Reckoning 为例）
g++ -std=c++11 -O2 \
    Dead_Reckoning_Test/Dead_Reckoning_Test/Dead_Reckoning_Test.cpp \
    -o build/Dead_Reckoning_Test

# 运行测试
./build/Dead_Reckoning_Test \
    data_set/Geolife_100k_with_id.csv \
    50.0 \
    output_DR.txt \
    -1
```

#### Windows 手动测试

```cmd
# 运行测试（使用预编译的 exe）
Dead_Reckoning_Test\x64\Debug\Dead_Reckoning_Test.exe ^
    data_set\Geolife_100k_with_id.csv ^
    50.0 ^
    output_DR.txt ^
    -1
```

## 测试参数说明

### 通用参数

所有算法（除 SQUISH-E 外）使用相同的参数格式：

```
<program> <input_csv> <epsilon> <output_file> [max_points]
```

- `input_csv`: 输入数据集文件路径
- `epsilon`: 误差阈值（单位：米）
- `output_file`: 输出文件路径
- `max_points`: 可选，限制读取的点数（-1 表示读取全部）

### SQUISH-E 特殊参数

```
<program> <input_csv> <ratio> <sed_threshold> <output_file> [max_points]
```

- `ratio`: 压缩比例参数
- `sed_threshold`: SED误差阈值（单位：米）

### 默认测试参数

- epsilon = 50.0 米（Dead_Reckoning, Douglas_Peucker, OPW_TR, VOLTCom）
- ratio = 2.0, sed = 50.0 米（SQUISH-E）
- max_points = -1（读取全部数据）

## 输出指标

测试脚本会生成详细的性能对比报告，包括：

### 压缩性能
- **原始点数**: 压缩前的GPS点数量
- **压缩后点数**: 压缩后保留的点数量
- **压缩比**: 压缩后点数 / 原始点数 × 100%

### 时间性能
- **压缩时间**: 平均每个点的压缩时间（微秒/点）
- **解压时间**: 平均每个点的解压缩时间（微秒/点）

### 准确性
- **平均误差**: 重建轨迹与原始轨迹的平均距离误差（米）
- **最大误差**: 最大的单点距离误差（米）

## 测试结果示例

```
==================================================================================
数据集: Geolife
==================================================================================

算法                   原始点数      压缩后点数    压缩比(%)    压缩时间(us/点)     解压时间(us/点)     平均误差(m)     最大误差(m)    
--------------------------------------------------------------------------------
Dead_Reckoning        100000       15234        15.23       0.1234           0.0567           12.34          45.67         
Douglas_Peucker       100000       12456        12.46       0.2345           0.0678           10.23          42.89         
OPW_TR               100000       14567        14.57       0.1456           0.0589           11.45          43.21         
SQUISH_E             100000       13789        13.79       0.1789           0.0612           11.89          44.56         
VOLTCom              100000       16234        16.23       0.1123           0.0545           13.12          46.78         
```

## 目录结构

```
TrajectoryCompression/
├── data_set/                          # 测试数据集
│   ├── Geolife_100k_with_id.csv
│   ├── Trajtory_100k_with_id.csv
│   └── WX_taxi_100k_with_id.csv
├── Dead_Reckoning_Test/               # Dead Reckoning 算法实现
├── DouglasPeuckerTest/                # Douglas-Peucker 算法实现
├── OPW-TR-Test/                       # OPW-TR 算法实现
├── SQUISH_E_Test/                     # SQUISH-E 算法实现
├── VOLTComTest/                       # VOLTCom 算法实现
├── build/                             # 编译输出目录 (macOS/Linux)
├── test_results/                      # 测试结果输出目录
├── run_tests.py                       # 自动化测试脚本
├── compile_all.sh                     # 编译脚本 (macOS/Linux)
└── README_TEST.md                     # 本文档
```

## 故障排查

### 编译错误

如果编译失败，请检查：

1. 是否安装了 g++ 编译器
   ```bash
   g++ --version
   ```

2. 是否有足够的权限
   ```bash
   chmod +x compile_all.sh
   ```

3. 源文件是否存在
   ```bash
   ls -la Dead_Reckoning_Test/Dead_Reckoning_Test/
   ```

### 运行错误

1. **找不到数据文件**: 确保数据集文件位于 `data_set/` 目录
2. **权限问题**: 使用 `chmod +x` 给予可执行权限
3. **内存不足**: 尝试使用 `max_points` 参数限制数据量

### Python 依赖

测试脚本只使用 Python 标准库，无需额外安装依赖。支持 Python 3.6+。

## 自定义测试

### 修改测试参数

编辑 `run_tests.py`，修改 epsilon 或其他参数：

```python
'params': lambda dataset, output: [dataset, '100.0', output, '-1']  # epsilon=100m
```

### 添加新数据集

在 `run_tests.py` 中添加数据集配置：

```python
DATASETS = [
    # ... 现有数据集 ...
    {
        'name': 'MyDataset',
        'file': 'data_set/my_dataset.csv'
    }
]
```

### 单独测试某个算法

```bash
# macOS/Linux
./build/Dead_Reckoning_Test data_set/Geolife_100k_with_id.csv 50.0 output.txt -1

# Windows
Dead_Reckoning_Test\x64\Debug\Dead_Reckoning_Test.exe data_set\Geolife_100k_with_id.csv 50.0 output.txt -1
```

## 性能优化建议

1. **编译优化**: 使用 `-O3` 而非 `-O2` 以获得更好的性能
2. **数据量控制**: 大数据集测试时可以使用 `max_points` 参数
3. **并行测试**: 可以修改脚本支持并行运行多个测试

## 引用和参考

如果使用本项目代码，请注明出处并引用相关论文：

1. Dead Reckoning: 航位推算轨迹压缩算法
2. Douglas-Peucker: Douglas, D. H., & Peucker, T. K. (1973)
3. OPW-TR: Meratnia, N., & de By, R. A. (2004)
4. SQUISH-E: Muckell et al. (2011)
5. VOLTCom: 向量轨迹压缩算法

## 许可证

请遵守各算法原始论文的使用条款和引用要求。

## 联系方式

如有问题或建议，请联系项目维护者。


