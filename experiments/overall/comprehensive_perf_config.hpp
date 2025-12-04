#ifndef COMPREHENSIVE_PERF_CONFIG_HPP
#define COMPREHENSIVE_PERF_CONFIG_HPP

#include <string>
#include <vector>

// 数据集配置
struct DatasetConfig {
    std::string name;
    std::string path;
};

// 数据集列表
const std::vector<DatasetConfig> kDataSetList = {
    // Multi-trajectory datasets (with trajectory_id)
    {"Geolife_100k", "../../data/Geolife_100k_with_id.csv"},
    {"Trajtory_100k", "../../data/Trajtory_100k_with_id.csv"},
    {"WX_taxi_100k", "../../data/WX_taxi_100k_with_id.csv"}
};

// 测试算法列表
const std::vector<std::string> kMethodList = {
    "LZ77",
    "Zstd",
    "Snappy",
    "SZ2",
    "Machete",
    "SimPiece",
    "Sprintz",
    "Deflate",
    "LZ4",
    "FPC",
    "Gorilla",
    "Chimp128",
    "Elf",
    "SerfQt",
    "SerfXOR"
};

// Overall 实验配置
constexpr double kMaxDiffOverall = 1.0E-5;  // 高精度：1e-5 度 ≈ 1.1 米

// Block Size 配置策略
// 批处理算法（LZ77, Zstd, Snappy, SZ2, Machete, SimPiece, Sprintz）：使用小 block_size
constexpr int kBatchProcessingBlockSize = 50;  // 批处理算法

// 流式算法（Deflate, LZ4, FPC, Gorilla, Chimp, Elf, SerfQt, SerfXOR, Simple）：使用数据集大小，但有上限
constexpr bool kUseDatasetSizeForStreaming = true;  // 是否使用数据集大小作为 block_size
constexpr int kStreamingBlockSizeMax = 10000;  // 流式算法的默认 block_size 上限（避免内存分配失败）

// 算法特定的 block_size 上限配置
// - 如果值为 -1，表示使用完整数据集大小（无上限）
// - 如果值为正数，表示该算法的 block_size 上限
const std::map<std::string, int> kAlgorithmSpecificBlockSizeMax = {
    // 能够处理大 block_size 的算法（使用完整数据集大小）
    {"SerfQt", -1},      // SerfQt 可以处理完整数据集
    {"Simple", -1},      // Simple 可以处理完整数据集
    {"Elf", -1},         // Elf 可以处理完整数据集
    {"Deflate", -1},     // Deflate 可以处理完整数据集
    {"LZ4", -1},         // LZ4 可以处理完整数据集
    
    // 需要限制 block_size 的算法
    {"Chimp128", 1000},  // Chimp128 在 block_size>1000 时会崩溃
    {"SerfXOR", 1000},   // SerfXOR 在大 block_size 时可能超时
    {"FPC", 10000},      // FPC 在 block_size>10000 时可能崩溃
    {"Gorilla", 10000}   // Gorilla 使用默认上限 10000
};

// SerfXOR 的 windows_size（固定值，不随 block_size 变化）
constexpr int kSerfXORWindowSize = 1000;

// Simple 算法配置
constexpr int kCoSTEvaluationWindow = 96;
constexpr bool kCoSTUseTimeWindow = false;

// SerfQt/SerfXOR 压缩模式配置
// 流式算法使用全数据集压缩（block_size = 数据集大小）
constexpr bool kUseFullDatasetCompression = true;  // true: 全数据集压缩, false: block-based 压缩

// 导出文件前缀和后缀
const std::string kExportFilePrefix = "comprehensive_perf_results_";
const std::string kExportFileSuffix = ".csv";

#endif  // COMPREHENSIVE_PERF_CONFIG_HPP

