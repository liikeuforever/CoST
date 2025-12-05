# CSV格式测试结果说明

## 📊 生成的CSV文件

完整测试会生成2个CSV文件，方便导入Excel、Python pandas等工具进行进一步分析：

### 1. test_results_summary.csv
**包含所有测试的详细结果**（15行数据）

| 列名 | 说明 | 单位 |
|------|------|------|
| 算法 | 算法名称 | - |
| 数据集 | 数据集名称 | - |
| 原始点数 | 压缩前的GPS点数量 | 点 |
| 压缩后点数 | 压缩后保留的点数量 | 点 |
| 压缩比(%) | 压缩后点数/原始点数 × 100 | % |
| 压缩时间(us/点) | 平均每个点的压缩时间 | 微秒/点 |
| 解压时间(us/点) | 平均每个点的解压时间 | 微秒/点 |
| 平均误差(m) | 重建轨迹的平均距离误差 | 米 |
| 最大误差(m) | 最大单点距离误差 | 米 |

**示例数据**:
```csv
算法,数据集,原始点数,压缩后点数,压缩比(%),压缩时间(us/点),解压时间(us/点),平均误差(m),最大误差(m)
Dead_Reckoning,Geolife,100000,8553,8.55,0.0309,0.0018,29.84,25453.07
Douglas_Peucker,Geolife,100000,4876,4.88,0.0093,0.0018,81.43,34025.96
OPW_TR,Geolife,100000,10392,10.39,0.3125,0.0016,13.98,49.99
...
```

### 2. test_results_average.csv
**包含各算法在所有数据集上的平均性能**（5行数据）

| 列名 | 说明 | 单位 |
|------|------|------|
| 算法 | 算法名称 | - |
| 平均压缩比(%) | 在3个数据集上的平均压缩比 | % |
| 平均压缩时间(us/点) | 平均压缩时间 | 微秒/点 |
| 平均解压时间(us/点) | 平均解压时间 | 微秒/点 |
| 平均误差(m) | 平均距离误差 | 米 |

**示例数据**:
```csv
算法,平均压缩比(%),平均压缩时间(us/点),平均解压时间(us/点),平均误差(m)
Dead_Reckoning,20.73,0.0328,0.0021,57.20
Douglas_Peucker,16.54,0.0110,0.0022,67.76
OPW_TR,31.25,0.3169,0.0017,13.13
SQUISH_E,24.28,0.1975,0.0021,15.18
VOLTCom,24.00,0.0685,0.0165,82.99
```

## 📈 如何使用CSV文件

### 方法1: Excel打开

1. 双击CSV文件，使用Excel打开
2. 可以进行排序、筛选、图表制作等操作
3. 适合快速查看和简单分析

### 方法2: Python pandas分析

```python
import pandas as pd
import matplotlib.pyplot as plt

# 读取详细结果
df_detail = pd.read_csv('test_results/test_results_summary.csv', encoding='utf-8')

# 读取平均结果
df_avg = pd.read_csv('test_results/test_results_average.csv', encoding='utf-8')

# 按压缩比排序
df_avg_sorted = df_avg.sort_values('平均压缩比(%)', ascending=False)
print(df_avg_sorted)

# 绘制压缩比对比图
plt.figure(figsize=(10, 6))
plt.bar(df_avg['算法'], df_avg['平均压缩比(%)'])
plt.xlabel('算法')
plt.ylabel('平均压缩比 (%)')
plt.title('各算法压缩比对比')
plt.xticks(rotation=45)
plt.tight_layout()
plt.savefig('compression_ratio_comparison.png')
plt.show()

# 绘制压缩时间对比图
plt.figure(figsize=(10, 6))
plt.bar(df_avg['算法'], df_avg['平均压缩时间(us/点)'])
plt.xlabel('算法')
plt.ylabel('平均压缩时间 (us/点)')
plt.title('各算法压缩时间对比')
plt.xticks(rotation=45)
plt.tight_layout()
plt.savefig('compression_time_comparison.png')
plt.show()

# 按数据集分组分析
for dataset in df_detail['数据集'].unique():
    df_dataset = df_detail[df_detail['数据集'] == dataset]
    print(f"\n数据集: {dataset}")
    print(df_dataset[['算法', '压缩比(%)', '平均误差(m)']].to_string(index=False))
```

### 方法3: R语言分析

```r
library(tidyverse)

# 读取数据
df_detail <- read_csv('test_results/test_results_summary.csv')
df_avg <- read_csv('test_results/test_results_average.csv')

# 可视化
ggplot(df_avg, aes(x = 算法, y = `平均压缩比(%)`)) +
  geom_bar(stat = "identity", fill = "steelblue") +
  theme_minimal() +
  labs(title = "各算法压缩比对比",
       x = "算法",
       y = "平均压缩比 (%)") +
  theme(axis.text.x = element_text(angle = 45, hjust = 1))

# 按数据集对比
ggplot(df_detail, aes(x = 算法, y = `压缩比(%)`, fill = 数据集)) +
  geom_bar(stat = "identity", position = "dodge") +
  theme_minimal() +
  labs(title = "各算法在不同数据集上的压缩比",
       x = "算法",
       y = "压缩比 (%)") +
  theme(axis.text.x = element_text(angle = 45, hjust = 1))
```

### 方法4: 在线工具分析

1. 访问 Google Sheets 或 Microsoft Excel Online
2. 导入CSV文件
3. 使用内置的图表工具进行可视化
4. 可以分享给团队成员

## 📊 快速分析示例

### 找出最优算法

**最高压缩比**:
```bash
# 在 test_results_average.csv 中查找
# OPW_TR: 31.25%
```

**最快压缩速度**:
```bash
# Douglas_Peucker: 0.0110 us/点
```

**最低误差**:
```bash
# OPW_TR: 13.13m
```

### 按数据集分析

从 `test_results_summary.csv` 中可以看到不同数据集的特点：

- **Geolife**: 压缩难度大（平均压缩比 7.55%）
- **Trajectory**: 中等压缩（平均压缩比 20.34%）
- **WX_taxi**: 压缩效果好（平均压缩比 42.19%）

## 🔧 自定义CSV输出

如果需要修改CSV格式，编辑 `run_tests.py` 中的 `generate_csv_report` 函数：

```python
def generate_csv_report(all_results, output_dir):
    csv_file = output_dir / "test_results_summary.csv"
    
    with open(csv_file, 'w', encoding='utf-8') as f:
        # 修改表头
        f.write("Algorithm,Dataset,Original,Compressed,Ratio,CompressTime,DecompressTime,AvgError,MaxError\n")
        
        # 修改数据格式
        for dataset in DATASETS:
            # ... 自定义输出格式
```

## 📝 注意事项

1. **编码**: CSV文件使用UTF-8编码，确保Excel能正确显示中文
2. **小数位数**: 
   - 压缩比、误差：保留2位小数
   - 时间：保留4位小数
3. **分隔符**: 使用逗号(,)作为分隔符
4. **缺失值**: 如果测试失败，显示为 "N/A"

## 🎯 数据验证

运行测试后，建议进行以下验证：

```bash
# 1. 查看CSV文件是否生成
ls -lh test_results/*.csv

# 2. 查看CSV内容
head test_results/test_results_summary.csv

# 3. 统计测试数量
wc -l test_results/test_results_summary.csv
# 应该显示 16 行（1行表头 + 15行数据）

# 4. 验证数据完整性
cat test_results/test_results_average.csv
# 应该显示 6 行（1行表头 + 5个算法）
```

## 📚 相关文档

- **测试说明.md** - 如何运行测试
- **README_TEST.md** - 技术细节
- **START_HERE.txt** - 快速开始指南

## 💡 常见应用场景

### 场景1: 学术论文图表
使用 `test_results_average.csv` 生成算法性能对比图

### 场景2: 算法选型
根据 `test_results_summary.csv` 分析不同数据集的表现

### 场景3: 性能优化
对比不同参数下的测试结果（需要多次运行测试）

### 场景4: 教学演示
使用CSV数据制作PPT图表

---

**最后更新**: 2025-11-18  
**测试版本**: epsilon=50m, 100k点/数据集


