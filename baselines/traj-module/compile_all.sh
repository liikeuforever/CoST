#!/bin/bash
# 编译所有轨迹压缩算法 (macOS/Linux)

echo "========================================"
echo "编译所有轨迹压缩算法"
echo "========================================"
echo ""

# 创建 build 目录
mkdir -p build

# 算法列表
declare -a algorithms=(
    "Dead_Reckoning_Test:Dead_Reckoning_Test/Dead_Reckoning_Test/Dead_Reckoning_Test.cpp"
    "Douglas_Peucker:DouglasPeuckerTest/DouglasPeuckerTest/DouglasPeuckerTest.cpp"
    "OPW_TR:OPW-TR-Test/OPW-TR-Test/OPW-TR-Test.cpp"
    "SQUISH_E:SQUISH_E_Test/SQUISH_E_Test/SQUISH_E_Test.cpp"
    "VOLTCom:VOLTComTest/VOLTComTest/VOLTComTest.cpp"
)

# 编译每个算法
success_count=0
fail_count=0

for algo_info in "${algorithms[@]}"; do
    IFS=':' read -r name source <<< "$algo_info"
    
    echo "[$((success_count + fail_count + 1))/${#algorithms[@]}] 正在编译 $name..."
    
    if [ ! -f "$source" ]; then
        echo "  ❌ 源文件不存在: $source"
        ((fail_count++))
        continue
    fi
    
    # 编译命令
    g++ -std=c++11 -O2 "$source" -o "build/$name" 2>&1 | head -20
    
    if [ ${PIPESTATUS[0]} -eq 0 ]; then
        echo "  ✓ 编译成功: build/$name"
        ((success_count++))
    else
        echo "  ❌ 编译失败"
        ((fail_count++))
    fi
    echo ""
done

echo "========================================"
echo "编译完成"
echo "========================================"
echo "成功: $success_count"
echo "失败: $fail_count"
echo ""

if [ $success_count -gt 0 ]; then
    echo "可执行文件位于 build/ 目录下"
    ls -lh build/
fi


