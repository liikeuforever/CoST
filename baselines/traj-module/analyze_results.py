#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CSV测试结果分析脚本
使用pandas和matplotlib分析轨迹压缩算法的性能
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

# 设置中文字体（macOS）
plt.rcParams['font.sans-serif'] = ['Arial Unicode MS', 'SimHei', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False

# 项目目录
PROJECT_ROOT = Path(__file__).parent.absolute()
RESULTS_DIR = PROJECT_ROOT / "test_results"
OUTPUT_DIR = PROJECT_ROOT / "test_results" / "charts"

def load_data():
    """加载CSV数据"""
    print("📊 正在加载测试数据...")
    
    # 读取详细结果
    csv_detail = RESULTS_DIR / "test_results_summary.csv"
    csv_avg = RESULTS_DIR / "test_results_average.csv"
    
    if not csv_detail.exists():
        print("❌ 找不到测试结果文件！")
        print("   请先运行: python3 run_tests.py")
        return None, None
    
    df_detail = pd.read_csv(csv_detail, encoding='utf-8')
    df_avg = pd.read_csv(csv_avg, encoding='utf-8')
    
    print(f"✓ 加载了 {len(df_detail)} 条详细记录")
    print(f"✓ 加载了 {len(df_avg)} 个算法的平均数据\n")
    
    return df_detail, df_avg

def create_output_dir():
    """创建输出目录"""
    OUTPUT_DIR.mkdir(exist_ok=True)
    print(f"✓ 图表输出目录: {OUTPUT_DIR}\n")

def plot_compression_ratio(df_avg):
    """绘制压缩比对比图"""
    print("📈 生成压缩比对比图...")
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    colors = ['#FF6B6B', '#4ECDC4', '#45B7D1', '#FFA07A', '#98D8C8']
    bars = ax.bar(df_avg['算法'], df_avg['平均压缩比(%)'], color=colors, alpha=0.8)
    
    # 添加数值标签
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}%',
                ha='center', va='bottom', fontsize=10)
    
    ax.set_xlabel('算法', fontsize=12)
    ax.set_ylabel('平均压缩比 (%)', fontsize=12)
    ax.set_title('各算法平均压缩比对比', fontsize=14, fontweight='bold')
    ax.grid(axis='y', alpha=0.3)
    plt.xticks(rotation=15)
    plt.tight_layout()
    
    output_file = OUTPUT_DIR / "1_compression_ratio.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"  ✓ 保存: {output_file}")
    plt.close()

def plot_compression_time(df_avg):
    """绘制压缩时间对比图"""
    print("📈 生成压缩时间对比图...")
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    colors = ['#FF6B6B', '#4ECDC4', '#45B7D1', '#FFA07A', '#98D8C8']
    bars = ax.bar(df_avg['算法'], df_avg['平均压缩时间(us/点)'], color=colors, alpha=0.8)
    
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.4f}',
                ha='center', va='bottom', fontsize=9)
    
    ax.set_xlabel('算法', fontsize=12)
    ax.set_ylabel('平均压缩时间 (us/点)', fontsize=12)
    ax.set_title('各算法平均压缩时间对比', fontsize=14, fontweight='bold')
    ax.grid(axis='y', alpha=0.3)
    plt.xticks(rotation=15)
    plt.tight_layout()
    
    output_file = OUTPUT_DIR / "2_compression_time.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"  ✓ 保存: {output_file}")
    plt.close()

def plot_error(df_avg):
    """绘制平均误差对比图"""
    print("📈 生成平均误差对比图...")
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    colors = ['#FF6B6B', '#4ECDC4', '#45B7D1', '#FFA07A', '#98D8C8']
    bars = ax.bar(df_avg['算法'], df_avg['平均误差(m)'], color=colors, alpha=0.8)
    
    for bar in bars:
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}m',
                ha='center', va='bottom', fontsize=10)
    
    ax.set_xlabel('算法', fontsize=12)
    ax.set_ylabel('平均误差 (m)', fontsize=12)
    ax.set_title('各算法平均误差对比', fontsize=14, fontweight='bold')
    ax.grid(axis='y', alpha=0.3)
    plt.xticks(rotation=15)
    plt.tight_layout()
    
    output_file = OUTPUT_DIR / "3_average_error.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"  ✓ 保存: {output_file}")
    plt.close()

def plot_by_dataset(df_detail):
    """按数据集绘制压缩比对比图"""
    print("📈 生成各数据集压缩比对比图...")
    
    fig, ax = plt.subplots(figsize=(12, 6))
    
    datasets = df_detail['数据集'].unique()
    algorithms = df_detail['算法'].unique()
    
    x = np.arange(len(algorithms))
    width = 0.25
    
    colors = ['#FF6B6B', '#4ECDC4', '#45B7D1']
    
    for i, dataset in enumerate(datasets):
        df_dataset = df_detail[df_detail['数据集'] == dataset]
        ratios = [df_dataset[df_dataset['算法'] == algo]['压缩比(%)'].values[0] 
                 for algo in algorithms]
        
        offset = width * (i - 1)
        bars = ax.bar(x + offset, ratios, width, label=dataset, 
                     color=colors[i], alpha=0.8)
    
    ax.set_xlabel('算法', fontsize=12)
    ax.set_ylabel('压缩比 (%)', fontsize=12)
    ax.set_title('各算法在不同数据集上的压缩比对比', fontsize=14, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(algorithms, rotation=15)
    ax.legend()
    ax.grid(axis='y', alpha=0.3)
    plt.tight_layout()
    
    output_file = OUTPUT_DIR / "4_compression_by_dataset.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"  ✓ 保存: {output_file}")
    plt.close()

def plot_comprehensive_comparison(df_avg):
    """绘制综合性能雷达图"""
    print("📈 生成综合性能雷达图...")
    
    # 归一化数据（越大越好）
    df_norm = df_avg.copy()
    
    # 压缩比：越大越好，直接使用
    df_norm['压缩比_norm'] = df_norm['平均压缩比(%)'] / df_norm['平均压缩比(%)'].max() * 100
    
    # 压缩时间：越小越好，取倒数
    df_norm['压缩速度_norm'] = (1 / df_norm['平均压缩时间(us/点)']) / (1 / df_norm['平均压缩时间(us/点)']).max() * 100
    
    # 解压时间：越小越好，取倒数
    df_norm['解压速度_norm'] = (1 / df_norm['平均解压时间(us/点)']) / (1 / df_norm['平均解压时间(us/点)']).max() * 100
    
    # 误差：越小越好，取倒数
    df_norm['准确性_norm'] = (1 / df_norm['平均误差(m)']) / (1 / df_norm['平均误差(m)']).max() * 100
    
    # 雷达图
    categories = ['压缩比', '压缩速度', '解压速度', '准确性']
    N = len(categories)
    
    angles = [n / float(N) * 2 * np.pi for n in range(N)]
    angles += angles[:1]
    
    fig, ax = plt.subplots(figsize=(10, 10), subplot_kw=dict(projection='polar'))
    
    colors = ['#FF6B6B', '#4ECDC4', '#45B7D1', '#FFA07A', '#98D8C8']
    
    for i, (idx, row) in enumerate(df_norm.iterrows()):
        values = [row['压缩比_norm'], row['压缩速度_norm'], 
                 row['解压速度_norm'], row['准确性_norm']]
        values += values[:1]
        
        ax.plot(angles, values, 'o-', linewidth=2, label=row['算法'], 
               color=colors[i], alpha=0.7)
        ax.fill(angles, values, alpha=0.15, color=colors[i])
    
    ax.set_xticks(angles[:-1])
    ax.set_xticklabels(categories, fontsize=11)
    ax.set_ylim(0, 100)
    ax.set_yticks([20, 40, 60, 80, 100])
    ax.set_yticklabels(['20', '40', '60', '80', '100'])
    ax.grid(True)
    ax.legend(loc='upper right', bbox_to_anchor=(1.3, 1.1))
    
    plt.title('算法综合性能对比（归一化）', fontsize=14, fontweight='bold', pad=20)
    plt.tight_layout()
    
    output_file = OUTPUT_DIR / "5_comprehensive_radar.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"  ✓ 保存: {output_file}")
    plt.close()

def print_statistics(df_detail, df_avg):
    """打印统计信息"""
    print("\n" + "="*60)
    print("📊 统计分析结果")
    print("="*60 + "\n")
    
    print("🏆 最优算法:")
    print(f"  - 最高压缩比: {df_avg.loc[df_avg['平均压缩比(%)'].idxmax(), '算法']} "
          f"({df_avg['平均压缩比(%)'].max():.2f}%)")
    print(f"  - 最快压缩: {df_avg.loc[df_avg['平均压缩时间(us/点)'].idxmin(), '算法']} "
          f"({df_avg['平均压缩时间(us/点)'].min():.4f} us/点)")
    print(f"  - 最快解压: {df_avg.loc[df_avg['平均解压时间(us/点)'].idxmin(), '算法']} "
          f"({df_avg['平均解压时间(us/点)'].min():.4f} us/点)")
    print(f"  - 最低误差: {df_avg.loc[df_avg['平均误差(m)'].idxmin(), '算法']} "
          f"({df_avg['平均误差(m)'].min():.2f} m)")
    
    print("\n📈 数据集特点:")
    for dataset in df_detail['数据集'].unique():
        df_ds = df_detail[df_detail['数据集'] == dataset]
        avg_ratio = df_ds['压缩比(%)'].mean()
        print(f"  - {dataset}: 平均压缩比 {avg_ratio:.2f}%")
    
    print("\n" + "="*60 + "\n")

def main():
    """主函数"""
    print("\n" + "="*60)
    print("CSV测试结果分析工具")
    print("="*60 + "\n")
    
    # 加载数据
    df_detail, df_avg = load_data()
    if df_detail is None:
        return
    
    # 创建输出目录
    create_output_dir()
    
    # 生成图表
    print("正在生成可视化图表...\n")
    plot_compression_ratio(df_avg)
    plot_compression_time(df_avg)
    plot_error(df_avg)
    plot_by_dataset(df_detail)
    plot_comprehensive_comparison(df_avg)
    
    # 打印统计信息
    print_statistics(df_detail, df_avg)
    
    print("✅ 所有图表已生成！")
    print(f"📁 图表保存位置: {OUTPUT_DIR}/")
    print("\n生成的图表:")
    for f in sorted(OUTPUT_DIR.glob("*.png")):
        print(f"  - {f.name}")

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"\n❌ 发生错误: {e}")
        import traceback
        traceback.print_exc()


