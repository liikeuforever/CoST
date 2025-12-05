#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
快速测试脚本 - 只测试前10000个点
"""

import subprocess
import os
import re
import time
from pathlib import Path
import sys
import platform

# 项目根目录
PROJECT_ROOT = Path(__file__).parent.absolute()

# 检测操作系统
IS_WINDOWS = platform.system() == 'Windows'
IS_MACOS = platform.system() == 'Darwin'
IS_LINUX = platform.system() == 'Linux'

# 根据操作系统选择可执行文件
def get_executable_path(base_name, exe_name=None):
    """根据操作系统返回可执行文件路径"""
    if IS_WINDOWS:
        return f"{base_name}/x64/Debug/{Path(base_name).name}.exe"
    else:
        if exe_name is None:
            exe_name = Path(base_name).name
        return f"build/{exe_name}"

# 算法配置 - 快速测试选择所有算法
ALGORITHMS = [
    {
        'name': 'Dead_Reckoning',
        'executable': get_executable_path('Dead_Reckoning_Test', 'Dead_Reckoning_Test'),
        'params': lambda dataset, output: [dataset, '1.0', output, '10000']  # epsilon=1m, 只测试10000个点
    },
    {
        'name': 'Douglas_Peucker',
        'executable': get_executable_path('DouglasPeuckerTest', 'Douglas_Peucker'),
        'params': lambda dataset, output: [dataset, '1.0', output, '10000']  # epsilon=1m
    },
    {
        'name': 'OPW_TR',
        'executable': get_executable_path('OPW-TR-Test', 'OPW_TR'),
        'params': lambda dataset, output: [dataset, '1.0', output, '10000']  # epsilon=1m
    },
    {
        'name': 'SQUISH_E',
        'executable': get_executable_path('SQUISH_E_Test', 'SQUISH_E'),
        'params': lambda dataset, output: [dataset, '2.0', '1.0', output, '10000']  # sed=1m
    },
    {
        'name': 'VOLTCom',
        'executable': get_executable_path('VOLTComTest', 'VOLTCom'),
        'params': lambda dataset, output: [dataset, '1.0', output, '10000']  # epsilon=1m
    },
]

# 数据集配置 - 快速测试只选择1个数据集
DATASETS = [
    {
        'name': 'Geolife',
        'file': 'data_set/Geolife_100k_with_id.csv'
    }
]

def parse_output_file(output_file):
    """解析算法输出文件，提取性能指标"""
    results = {
        'original_points': 0,
        'compressed_points': 0,
        'compression_ratio': 0.0,
        'avg_compress_time': 0.0,
        'avg_decompress_time': 0.0,
        'avg_error': 0.0,
        'max_error': 0.0
    }
    
    if not os.path.exists(output_file):
        return results
    
    try:
        with open(output_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
            patterns = {
                'original_points': r'Original Points:\s*(\d+)',
                'compressed_points': r'(?:Compressed Points|Compressed Segments|Simplified Points):\s*(\d+)',
                'compression_ratio': r'Compression Ratio:\s*([\d.]+)%',
                'avg_compress_time': r'Average Time per Point:\s*([\d.]+)\s*us/point',
                'avg_decompress_time': r'--- Decompression.*?Average Time per Point:\s*([\d.]+)\s*us/point',
                'avg_error': r'Average Error:\s*([\d.]+)\s*m',
                'max_error': r'Maximum Error:\s*([\d.]+)\s*m'
            }
            
            for key, pattern in patterns.items():
                match = re.search(pattern, content, re.DOTALL)
                if match:
                    results[key] = float(match.group(1))
    except:
        pass
    
    return results

def run_algorithm(algorithm, dataset, output_dir):
    """运行单个算法对单个数据集的测试"""
    algo_name = algorithm['name']
    dataset_name = dataset['name']
    
    exe_path = PROJECT_ROOT / algorithm['executable']
    
    if not exe_path.exists():
        print(f"  ⚠️  可执行文件不存在: {exe_path}")
        return None
    
    dataset_path = PROJECT_ROOT / dataset['file']
    if not dataset_path.exists():
        print(f"  ⚠️  数据集不存在: {dataset_path}")
        return None
    
    output_file = output_dir / f"{algo_name}_{dataset_name}_output.txt"
    params = algorithm['params'](str(dataset_path), str(output_file))
    cmd = [str(exe_path)] + params
    
    print(f"  正在运行: {algo_name} on {dataset_name} (10000点)...")
    
    try:
        start_time = time.time()
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=60,
            encoding='utf-8',
            errors='ignore'
        )
        end_time = time.time()
        
        if result.returncode != 0:
            print(f"  ❌ 运行失败")
            return None
        
        results = parse_output_file(output_file)
        results['wall_time'] = end_time - start_time
        
        print(f"  ✓ 完成 (耗时: {results['wall_time']:.2f}秒)")
        print(f"    压缩比: {results['compression_ratio']:.2f}%")
        print(f"    平均压缩时间: {results['avg_compress_time']:.4f} us/点")
        print(f"    平均解压时间: {results['avg_decompress_time']:.4f} us/点")
        
        return results
        
    except Exception as e:
        print(f"  ❌ 异常: {e}")
        return None

def main():
    """主函数"""
    print("\n" + "=" * 80)
    print("快速测试 - 每个数据集测试10000个点")
    print("=" * 80 + "\n")
    
    output_dir = PROJECT_ROOT / "test_results"
    output_dir.mkdir(exist_ok=True)
    
    all_results = {}
    
    for dataset in DATASETS:
        print(f"\n{'='*80}")
        print(f"测试数据集: {dataset['name']}")
        print(f"{'='*80}\n")
        
        for algorithm in ALGORITHMS:
            result = run_algorithm(algorithm, dataset, output_dir)
            if result:
                all_results[(algorithm['name'], dataset['name'])] = result
            print()
    
    # 输出汇总
    print("\n" + "=" * 80)
    print("测试结果汇总")
    print("=" * 80 + "\n")
    
    print(f"{'算法':<20} {'压缩比(%)':<12} {'压缩时间(us/点)':<18} {'解压时间(us/点)':<18}")
    print("-" * 70)
    
    for algo in ALGORITHMS:
        key = (algo['name'], 'Geolife')
        if key in all_results:
            r = all_results[key]
            print(f"{algo['name']:<20} {r['compression_ratio']:<12.2f} "
                  f"{r['avg_compress_time']:<18.4f} {r['avg_decompress_time']:<18.4f}")
        else:
            print(f"{algo['name']:<20} {'失败':<12} {'N/A':<18} {'N/A':<18}")
    
    # 生成CSV文件
    csv_file = output_dir / "quick_test_results.csv"
    with open(csv_file, 'w', encoding='utf-8') as f:
        f.write("算法,数据集,原始点数,压缩后点数,压缩比(%),压缩时间(us/点),解压时间(us/点),平均误差(m),最大误差(m)\n")
        for algo in ALGORITHMS:
            key = (algo['name'], 'Geolife')
            if key in all_results:
                r = all_results[key]
                f.write(f"{algo['name']},Geolife,"
                       f"{int(r['original_points'])},{int(r['compressed_points'])},"
                       f"{r['compression_ratio']:.2f},"
                       f"{r['avg_compress_time']:.4f},{r['avg_decompress_time']:.4f},"
                       f"{r['avg_error']:.2f},{r['max_error']:.2f}\n")
    
    print(f"\n✓ CSV结果已保存: {csv_file}")
    print("\n✓ 快速测试完成！")
    print("如需完整测试，请运行: python3 run_tests.py")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  测试被用户中断")
        sys.exit(1)

