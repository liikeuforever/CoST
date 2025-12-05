#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
轨迹压缩算法测试脚本
对多个数据集运行所有算法，并汇总对比结果
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
        # macOS/Linux: 使用指定的可执行文件名
        if exe_name is None:
            exe_name = Path(base_name).name
        return f"build/{exe_name}"

# 算法配置
ALGORITHMS = [
    {
        'name': 'Dead_Reckoning',
        'executable': get_executable_path('Dead_Reckoning_Test', 'Dead_Reckoning_Test'),
        'source_dir': 'Dead_Reckoning_Test/Dead_Reckoning_Test',
        'source_file': 'Dead_Reckoning_Test.cpp',
        'params': lambda dataset, output: [dataset, '1.0', output, '-1']  # epsilon=1m
    },
    {
        'name': 'Douglas_Peucker',
        'executable': get_executable_path('DouglasPeuckerTest', 'Douglas_Peucker'),
        'source_dir': 'DouglasPeuckerTest/DouglasPeuckerTest',
        'source_file': 'DouglasPeuckerTest.cpp',
        'params': lambda dataset, output: [dataset, '1.0', output, '-1']  # epsilon=1m
    },
    {
        'name': 'OPW_TR',
        'executable': get_executable_path('OPW-TR-Test', 'OPW_TR'),
        'source_dir': 'OPW-TR-Test/OPW-TR-Test',
        'source_file': 'OPW-TR-Test.cpp',
        'params': lambda dataset, output: [dataset, '1.0', output, '-1']  # epsilon=1m
    },
    {
        'name': 'SQUISH_E',
        'executable': get_executable_path('SQUISH_E_Test', 'SQUISH_E'),
        'source_dir': 'SQUISH_E_Test/SQUISH_E_Test',
        'source_file': 'SQUISH_E_Test.cpp',
        'params': lambda dataset, output: [dataset, '2.0', '1.0', output, '-1']  # ratio=2, sed=1m
    },
    {
        'name': 'VOLTCom',
        'executable': get_executable_path('VOLTComTest', 'VOLTCom'),
        'source_dir': 'VOLTComTest/VOLTComTest',
        'source_file': 'VOLTComTest.cpp',
        'params': lambda dataset, output: [dataset, '1.0', output, '-1']  # epsilon=1m
    }
]

# 数据集配置
DATASETS = [
    {
        'name': 'Geolife',
        'file': 'data_set/Geolife_100k_with_id.csv'
    },
    {
        'name': 'Trajectory',
        'file': 'data_set/Trajtory_100k_with_id.csv'
    },
    {
        'name': 'WX_taxi',
        'file': 'data_set/WX_taxi_100k_with_id.csv'
    }
]

def parse_output_file(output_file):
    """
    解析算法输出文件，提取性能指标
    """
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
        print(f"  警告: 输出文件不存在 {output_file}")
        return results
    
    try:
        with open(output_file, 'r', encoding='utf-8') as f:
            content = f.read()
            
            # 提取各项指标
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
    
    except Exception as e:
        print(f"  错误: 解析输出文件失败 {output_file}: {e}")
    
    return results

def compile_algorithm(algorithm):
    """
    在 macOS/Linux 上编译 C++ 源代码
    """
    if IS_WINDOWS:
        return True  # Windows 上使用预编译的 exe
    
    algo_name = algorithm['name']
    source_dir = PROJECT_ROOT / algorithm['source_dir']
    source_file = source_dir / algorithm['source_file']
    
    if not source_file.exists():
        print(f"  ⚠️  源文件不存在: {source_file}")
        return False
    
    # 创建 build 目录
    build_dir = PROJECT_ROOT / "build"
    build_dir.mkdir(exist_ok=True)
    
    # 输出可执行文件路径
    output_exe = build_dir / algo_name
    
    # 如果已编译且较新，跳过
    if output_exe.exists() and output_exe.stat().st_mtime > source_file.stat().st_mtime:
        return True
    
    print(f"  📦 正在编译 {algo_name}...")
    
    # 编译命令
    compile_cmd = [
        'g++',
        '-std=c++11',
        '-O2',
        str(source_file),
        '-o', str(output_exe)
    ]
    
    try:
        result = subprocess.run(
            compile_cmd,
            capture_output=True,
            text=True,
            timeout=60
        )
        
        if result.returncode != 0:
            print(f"  ❌ 编译失败:")
            print(f"     {result.stderr[:500]}")
            return False
        
        print(f"  ✓ 编译成功: {output_exe}")
        return True
        
    except Exception as e:
        print(f"  ❌ 编译异常: {e}")
        return False

def run_algorithm(algorithm, dataset, output_dir):
    """
    运行单个算法对单个数据集的测试
    """
    algo_name = algorithm['name']
    dataset_name = dataset['name']
    
    # 在非 Windows 平台上先编译
    if not IS_WINDOWS:
        if not compile_algorithm(algorithm):
            print(f"  ⚠️  编译失败，跳过测试")
            return None
    
    # 构建可执行文件路径
    exe_path = PROJECT_ROOT / algorithm['executable']
    
    # 检查可执行文件是否存在
    if not exe_path.exists():
        print(f"  ⚠️  可执行文件不存在: {exe_path}")
        return None
    
    # 构建数据集路径
    dataset_path = PROJECT_ROOT / dataset['file']
    if not dataset_path.exists():
        print(f"  ⚠️  数据集不存在: {dataset_path}")
        return None
    
    # 构建输出文件路径
    output_file = output_dir / f"{algo_name}_{dataset_name}_output.txt"
    
    # 构建命令参数
    params = algorithm['params'](str(dataset_path), str(output_file))
    cmd = [str(exe_path)] + params
    
    print(f"  正在运行: {algo_name} on {dataset_name}...")
    
    try:
        # 运行算法
        start_time = time.time()
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=300,  # 5分钟超时
            encoding='utf-8',
            errors='ignore'
        )
        end_time = time.time()
        
        if result.returncode != 0:
            print(f"  ❌ 运行失败 (返回码: {result.returncode})")
            print(f"     错误信息: {result.stderr[:200]}")
            return None
        
        # 解析输出文件
        results = parse_output_file(output_file)
        results['wall_time'] = end_time - start_time
        
        print(f"  ✓ 完成 (耗时: {results['wall_time']:.2f}秒)")
        print(f"    压缩比: {results['compression_ratio']:.2f}%")
        print(f"    平均压缩时间: {results['avg_compress_time']:.4f} us/点")
        print(f"    平均解压时间: {results['avg_decompress_time']:.4f} us/点")
        
        return results
        
    except subprocess.TimeoutExpired:
        print(f"  ⏱️  超时 (>5分钟)")
        return None
    except Exception as e:
        print(f"  ❌ 异常: {e}")
        return None

def generate_csv_report(all_results, output_dir):
    """
    生成CSV格式的汇总报告
    """
    csv_file = output_dir / "test_results_summary.csv"
    
    with open(csv_file, 'w', encoding='utf-8') as f:
        # 写入CSV表头
        f.write("算法,数据集,原始点数,压缩后点数,压缩比(%),压缩时间(us/点),解压时间(us/点),平均误差(m),最大误差(m)\n")
        
        # 写入每个测试的结果
        for dataset in DATASETS:
            dataset_name = dataset['name']
            for algo in ALGORITHMS:
                algo_name = algo['name']
                key = (algo_name, dataset_name)
                
                if key in all_results:
                    r = all_results[key]
                    f.write(f"{algo_name},{dataset_name},"
                           f"{int(r['original_points'])},{int(r['compressed_points'])},"
                           f"{r['compression_ratio']:.2f},"
                           f"{r['avg_compress_time']:.4f},{r['avg_decompress_time']:.4f},"
                           f"{r['avg_error']:.2f},{r['max_error']:.2f}\n")
                else:
                    f.write(f"{algo_name},{dataset_name},N/A,N/A,N/A,N/A,N/A,N/A,N/A\n")
    
    # 生成算法平均性能CSV
    csv_avg_file = output_dir / "test_results_average.csv"
    
    with open(csv_avg_file, 'w', encoding='utf-8') as f:
        f.write("算法,平均压缩比(%),平均压缩时间(us/点),平均解压时间(us/点),平均误差(m)\n")
        
        for algo in ALGORITHMS:
            algo_name = algo['name']
            algo_results = [all_results[(algo_name, ds['name'])] 
                          for ds in DATASETS 
                          if (algo_name, ds['name']) in all_results]
            
            if algo_results:
                avg_ratio = sum(r['compression_ratio'] for r in algo_results) / len(algo_results)
                avg_comp_time = sum(r['avg_compress_time'] for r in algo_results) / len(algo_results)
                avg_decomp_time = sum(r['avg_decompress_time'] for r in algo_results) / len(algo_results)
                avg_err = sum(r['avg_error'] for r in algo_results) / len(algo_results)
                
                f.write(f"{algo_name},{avg_ratio:.2f},{avg_comp_time:.4f},"
                       f"{avg_decomp_time:.4f},{avg_err:.2f}\n")
            else:
                f.write(f"{algo_name},N/A,N/A,N/A,N/A\n")
    
    print(f"\n✓ CSV报告已生成:")
    print(f"  - {csv_file}")
    print(f"  - {csv_avg_file}")
    return csv_file, csv_avg_file

def generate_report(all_results, output_dir):
    """
    生成汇总报告
    """
    report_file = output_dir / "test_results_summary.txt"
    
    with open(report_file, 'w', encoding='utf-8') as f:
        f.write("=" * 100 + "\n")
        f.write("轨迹压缩算法性能对比测试报告\n")
        f.write("=" * 100 + "\n\n")
        f.write(f"测试时间: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(f"测试参数: epsilon=1m (Dead_Reckoning, Douglas_Peucker, OPW_TR, VOLTCom)\n")
        f.write(f"          ratio=2, sed=1m (SQUISH_E)\n\n")
        
        # 按数据集分组输出
        for dataset in DATASETS:
            dataset_name = dataset['name']
            f.write("=" * 100 + "\n")
            f.write(f"数据集: {dataset_name}\n")
            f.write("=" * 100 + "\n\n")
            
            # 表头
            f.write(f"{'算法':<20} {'原始点数':<12} {'压缩后点数':<12} {'压缩比(%)':<12} "
                   f"{'压缩时间(us/点)':<18} {'解压时间(us/点)':<18} {'平均误差(m)':<15} {'最大误差(m)':<15}\n")
            f.write("-" * 140 + "\n")
            
            # 输出每个算法的结果
            for algo in ALGORITHMS:
                algo_name = algo['name']
                key = (algo_name, dataset_name)
                
                if key in all_results:
                    r = all_results[key]
                    f.write(f"{algo_name:<20} "
                           f"{int(r['original_points']):<12} "
                           f"{int(r['compressed_points']):<12} "
                           f"{r['compression_ratio']:<12.2f} "
                           f"{r['avg_compress_time']:<18.4f} "
                           f"{r['avg_decompress_time']:<18.4f} "
                           f"{r['avg_error']:<15.2f} "
                           f"{r['max_error']:<15.2f}\n")
                else:
                    f.write(f"{algo_name:<20} {'N/A':<12} {'N/A':<12} {'N/A':<12} "
                           f"{'N/A':<18} {'N/A':<18} {'N/A':<15} {'N/A':<15}\n")
            
            f.write("\n")
        
        # 汇总对比（按算法）
        f.write("\n" + "=" * 100 + "\n")
        f.write("算法性能汇总（所有数据集平均）\n")
        f.write("=" * 100 + "\n\n")
        
        f.write(f"{'算法':<20} {'平均压缩比(%)':<18} {'平均压缩时间(us/点)':<22} "
               f"{'平均解压时间(us/点)':<22} {'平均误差(m)':<15}\n")
        f.write("-" * 100 + "\n")
        
        for algo in ALGORITHMS:
            algo_name = algo['name']
            
            # 计算该算法在所有数据集上的平均值
            algo_results = [all_results[(algo_name, ds['name'])] 
                          for ds in DATASETS 
                          if (algo_name, ds['name']) in all_results]
            
            if algo_results:
                avg_ratio = sum(r['compression_ratio'] for r in algo_results) / len(algo_results)
                avg_comp_time = sum(r['avg_compress_time'] for r in algo_results) / len(algo_results)
                avg_decomp_time = sum(r['avg_decompress_time'] for r in algo_results) / len(algo_results)
                avg_err = sum(r['avg_error'] for r in algo_results) / len(algo_results)
                
                f.write(f"{algo_name:<20} "
                       f"{avg_ratio:<18.2f} "
                       f"{avg_comp_time:<22.4f} "
                       f"{avg_decomp_time:<22.4f} "
                       f"{avg_err:<15.2f}\n")
            else:
                f.write(f"{algo_name:<20} {'N/A':<18} {'N/A':<22} {'N/A':<22} {'N/A':<15}\n")
        
        f.write("\n" + "=" * 100 + "\n")
        f.write("测试完成\n")
        f.write("=" * 100 + "\n")
    
    print(f"\n✓ 汇总报告已生成: {report_file}")
    
    # 同时在控制台输出
    with open(report_file, 'r', encoding='utf-8') as f:
        print("\n" + f.read())

def main():
    """
    主函数
    """
    print("\n" + "=" * 100)
    print("轨迹压缩算法性能测试")
    print("=" * 100 + "\n")
    
    # 创建输出目录
    output_dir = PROJECT_ROOT / "test_results"
    output_dir.mkdir(exist_ok=True)
    print(f"输出目录: {output_dir}\n")
    
    # 存储所有测试结果
    all_results = {}
    
    # 运行所有测试
    total_tests = len(ALGORITHMS) * len(DATASETS)
    current_test = 0
    
    for dataset in DATASETS:
        print(f"\n{'='*100}")
        print(f"测试数据集: {dataset['name']}")
        print(f"{'='*100}\n")
        
        for algorithm in ALGORITHMS:
            current_test += 1
            print(f"[{current_test}/{total_tests}] {algorithm['name']} on {dataset['name']}")
            
            result = run_algorithm(algorithm, dataset, output_dir)
            
            if result:
                all_results[(algorithm['name'], dataset['name'])] = result
            
            print()
    
    # 生成汇总报告
    print("\n" + "=" * 100)
    print("生成汇总报告...")
    print("=" * 100)
    
    # 生成TXT格式报告
    generate_report(all_results, output_dir)
    
    # 生成CSV格式报告
    generate_csv_report(all_results, output_dir)
    
    print("\n✓ 所有测试完成！")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  测试被用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n\n❌ 发生错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

