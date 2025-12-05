/**
 * CoST Window Sensitivity Test
 * 
 *  CoST 
 * ：Serf-QT vs CoST
 * ：Trajectory (100k points)
 * ：
 *   1.  (error_max): 1e-6  1e-1
 *   2. : 1S, 5S, 10S, 20S, 40S
 *   3.  (window_size): 12  1020
 */

#include "algorithm/cost_compressor.h"
#include "baselines/serf/serf_qt_compressor.h"
#include "baselines/serf/serf_qt_decompressor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <chrono>
#include <iomanip>

using CoSTGpsPoint = CoSTCompressor::GpsPoint;

// (comment removed)
uint64_t ParseTimestamp(const std::string& ts_str) {
    std::tm tm = {};
    std::istringstream ss(ts_str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) return 0;
    return static_cast<uint64_t>(std::mktime(&tm));
}

// CSVGPS
std::vector<CoSTGpsPoint> LoadGpsDataFromCSV(const std::string& filename, int max_points = -1) {
    std::vector<CoSTGpsPoint> points;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << ": " << filename << std::endl;
        return points;
    }
    
    std::string line;
    int count = 0;
    
 // CSV header
    std::getline(file, line);
    
    while (std::getline(file, line) && (max_points < 0 || count < max_points)) {
        std::stringstream ss(line);
        std::string lon_str, lat_str, ts_str;
        
        if (std::getline(ss, lon_str, ',') && std::getline(ss, lat_str, ',')) {
            try {
                double longitude = std::stod(lon_str);
                double latitude = std::stod(lat_str);
                uint64_t timestamp = 0;
                
                if (std::getline(ss, ts_str, ',')) {
                    timestamp = ParseTimestamp(ts_str);
                }
                
                points.emplace_back(longitude, latitude, timestamp);
                count++;
            } catch (...) {
                continue;
            }
        }
    }
    
    std::cout << " " << points.size() << " GPS" << std::endl;
    return points;
}

// (comment removed)
struct DatasetConfig {
    std::string name;
    std::string path;
    
    DatasetConfig(const std::string& n, const std::string& p) : name(n), path(p) {}
};

// (comment removed)
struct TestResult {
    double compression_ratio;  //  (CR = compressed_bits / original_bits)
    double compress_time_us;   //  (μs/)
    double decompress_time_us; //  (μs/)
};

// Serf-QT
TestResult TestSerfQt(const std::vector<CoSTGpsPoint>& data, double epsilon) {
    TestResult result;
    
    // (comment removed)
    auto compress_start = std::chrono::steady_clock::now();
    SerfQtCompressor qt_lon(data.size(), epsilon);
    SerfQtCompressor qt_lat(data.size(), epsilon);
    for (const auto& point : data) {
        qt_lon.AddValue(point.longitude);
        qt_lat.AddValue(point.latitude);
    }
    qt_lon.Close();
    qt_lat.Close();
    auto compress_end = std::chrono::steady_clock::now();
    
    // (comment removed)
    auto decompress_start = std::chrono::steady_clock::now();
    auto qt_lon_data = qt_lon.compressed_bytes();
    auto qt_lat_data = qt_lat.compressed_bytes();
    SerfQtDecompressor qt_lon_dec;
    SerfQtDecompressor qt_lat_dec;
    auto qt_lon_values = qt_lon_dec.Decompress(qt_lon_data);
    auto qt_lat_values = qt_lat_dec.Decompress(qt_lat_data);
    auto decompress_end = std::chrono::steady_clock::now();
    
    // (comment removed)
    int compressed_bits = qt_lon.get_compressed_size_in_bits() + qt_lat.get_compressed_size_in_bits();
    int original_bits = data.size() * 128;  // 2 doubles = 128 bits
    result.compression_ratio = static_cast<double>(compressed_bits) / original_bits;
    
    // (comment removed)
    result.compress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        compress_end - compress_start).count() / static_cast<double>(data.size());
    result.decompress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        decompress_end - decompress_start).count() / static_cast<double>(data.size());
    
    return result;
}

// Simple
TestResult TestCoST(const std::vector<CoSTGpsPoint>& data, double epsilon, int window_size) {
    TestResult result;
    
    // (comment removed)
    auto compress_start = std::chrono::steady_clock::now();
    CoSTCompressor compressor(
        data.size(), 
        epsilon,
        window_size,
        false  // use_time_window = false
    );
    for (const auto& point : data) {
        compressor.AddGpsPoint(point);
    }
    compressor.Close();
    auto compress_end = std::chrono::steady_clock::now();
    
    // (comment removed)
    Array<uint8_t> compressed = compressor.GetCompressedData();
    auto decompress_start = std::chrono::steady_clock::now();
    CoSTDecompressor decompressor(compressed.begin(), compressed.length());
    std::vector<CoSTGpsPoint> decompressed;
    CoSTGpsPoint point;
    for (size_t i = 0; i < data.size(); ++i) {
        if (decompressor.ReadNextPoint(point)) {
            decompressed.push_back(point);
        } else {
            break;
        }
    }
    auto decompress_end = std::chrono::steady_clock::now();
    
 // （ timestamp bits）
    auto stats = compressor.GetStats();
    int compressed_bits = stats.total_bits - stats.timestamp_bits;
    int original_bits = data.size() * 128;  // 2 doubles = 128 bits
    result.compression_ratio = static_cast<double>(compressed_bits) / original_bits;
    
    // (comment removed)
    result.compress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        compress_end - compress_start).count() / static_cast<double>(data.size());
    result.decompress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        decompress_end - decompress_start).count() / static_cast<double>(data.size());
    
    return result;
}

// 1: 
void TestErrorThreshold(const std::vector<CoSTGpsPoint>& data, std::ofstream& csv_file) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "1:  ()" << std::endl;
    std::cout << "========================================" << std::endl;
    
 // （）：0.1m=1e-6, 0.5m=5e-6, 1m=1e-5, 5m=5e-5, 10m=1e-4, 20m=2e-4, 30m=3e-4
    std::vector<double> error_thresholds = {1e-6, 5e-6, 1e-5, 5e-5, 1e-4, 2e-4, 3e-4};
    std::vector<std::string> error_labels = {"0.1m", "0.5m", "1m", "5m", "10m", "20m", "30m"};
    const int window_size = 96;  // 
    
    csv_file << "\nAverage Compression Ratio,,,Average Compression Time (μs/pt),,,Average Decompression Time (μs/pt),,\n";
    csv_file << "error_max,Serf-QT,CoST,error_max,Serf-QT,CoST,error_max,Serf-QT,CoST\n";
    
    for (size_t i = 0; i < error_thresholds.size(); ++i) {
        double epsilon = error_thresholds[i];
        std::string label = error_labels[i];
        
        std::cout << "\n error_max = " << label << " (" << std::scientific << epsilon << ")" << std::endl;
        
        auto qt_result = TestSerfQt(data, epsilon);
        auto cost_result = TestCoST(data, epsilon, window_size);
        
        std::cout << "  Serf-QT: CR=" << std::fixed << std::setprecision(6) << qt_result.compression_ratio
                  << ", =" << qt_result.compress_time_us << " μs/pt"
                  << ", =" << qt_result.decompress_time_us << " μs/pt" << std::endl;
        std::cout << "  CoST:  CR=" << cost_result.compression_ratio
                  << ", =" << cost_result.compress_time_us << " μs/pt"
                  << ", =" << cost_result.decompress_time_us << " μs/pt" << std::endl;
        
        csv_file << label << ","
                 << std::fixed << std::setprecision(6)
                 << qt_result.compression_ratio << "," << cost_result.compression_ratio << ","
                 << label << ","
                 << std::fixed << std::setprecision(6)
                 << qt_result.compress_time_us << "," << cost_result.compress_time_us << ","
                 << label << ","
                 << std::fixed << std::setprecision(6)
                 << qt_result.decompress_time_us << "," << cost_result.decompress_time_us << "\n";
    }
}

// 2: （CSV）
void TestSamplingInterval(std::ofstream& csv_file) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "2: " << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string base_path = "../../data/";
    
    // (comment removed)
    std::vector<DatasetConfig> datasets = {
        DatasetConfig("1S", base_path + "Trajtory_100k_with_id.csv"),
        DatasetConfig("5S", base_path + "Trajtory_100k_with_id_downsample_5x.csv"),
        DatasetConfig("10S", base_path + "Trajtory_100k_with_id_downsample_10x.csv"),
        DatasetConfig("20S", base_path + "Trajtory_100k_with_id_downsample_20x.csv"),
        DatasetConfig("40S", base_path + "Trajtory_100k_with_id_downsample_40x.csv")
    };
    
    const double epsilon = 1e-5;
    const int window_size = 96;
    
    csv_file << "\nAverage Compression Ratio,,,Average Compression Time (μs/pt),,,Average Decompression Time (μs/pt),,\n";
    csv_file << "Dataset,Serf-QT,CoST,Dataset,Serf-QT,CoST,Dataset,Serf-QT,CoST\n";
    
    for (const auto& dataset : datasets) {
        std::cout << "\n  = " << dataset.name << std::endl;
        
        // (comment removed)
        auto data = LoadGpsDataFromCSV(dataset.path, -1);
        if (data.empty()) {
            std::cerr << "  ❌ : " << dataset.path << std::endl;
            continue;
        }
        
        std::cout << "  : " << data.size() << std::endl;
        
        auto qt_result = TestSerfQt(data, epsilon);
        auto cost_result = TestCoST(data, epsilon, window_size);
        
        std::cout << "  Serf-QT: CR=" << std::fixed << std::setprecision(6) << qt_result.compression_ratio
                  << ", =" << qt_result.compress_time_us << " μs/pt"
                  << ", =" << qt_result.decompress_time_us << " μs/pt" << std::endl;
        std::cout << "  CoST:  CR=" << cost_result.compression_ratio
                  << ", =" << cost_result.compress_time_us << " μs/pt"
                  << ", =" << cost_result.decompress_time_us << " μs/pt" << std::endl;
        
        csv_file << dataset.name << ","
                 << std::fixed << std::setprecision(6)
                 << qt_result.compression_ratio << "," << cost_result.compression_ratio << ","
                 << dataset.name << ","
                 << qt_result.compress_time_us << "," << cost_result.compress_time_us << ","
                 << dataset.name << ","
                 << qt_result.decompress_time_us << "," << cost_result.decompress_time_us << "\n";
    }
}

// 3: 
void TestWindowSize(const std::vector<CoSTGpsPoint>& data, std::ofstream& csv_file) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "3: " << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::vector<int> window_sizes = {12, 96, 180, 264, 348, 432, 516, 600, 684, 768, 852, 936, 1020};
    const double epsilon = 1e-5;
    
    csv_file << "\nAverage Compression Ratio,,,Average Compression Time (μs/pt),,,Average Decompression Time (μs/pt),,\n";
    csv_file << "window_size,,CoST,window_size,,CoST,window_size,,CoST\n";
    
    for (int window_size : window_sizes) {
        std::cout << "\n window_size = " << window_size << std::endl;
        
        auto cost_result = TestCoST(data, epsilon, window_size);
        
        std::cout << "  CoST: CR=" << std::fixed << std::setprecision(6) << cost_result.compression_ratio
                  << ", =" << cost_result.compress_time_us << " μs/pt"
                  << ", =" << cost_result.decompress_time_us << " μs/pt" << std::endl;
        
        csv_file << window_size << ",,"
                 << std::fixed << std::setprecision(6)
                 << cost_result.compression_ratio << ","
                 << window_size << ",,"
                 << cost_result.compress_time_us << ","
                 << window_size << ",,"
                 << cost_result.decompress_time_us << "\n";
    }
}

int main() {
    std::cout << "CoST Window Sensitivity Test" << std::endl;
    std::cout << "==============================" << std::endl;
    
    // Load Trajectory dataset (100k points with timestamp)
    std::string dataset_path = "../../data/Trajtory_100k_with_id.csv";
    auto full_data = LoadGpsDataFromCSV(dataset_path, 100000);
    
    if (full_data.empty()) {
        std::cerr << "！" << std::endl;
        return 1;
    }
    
 // CSV
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream csv_filename;
    csv_filename << "cost_window_sensitivity_results_" 
                 << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S") 
                 << ".csv";
    
    std::ofstream csv_file(csv_filename.str());
    if (!csv_file.is_open()) {
        std::cerr << "CSV！" << std::endl;
        return 1;
    }
    
    csv_file << "CoST Window Sensitivity Test Results\n";
    csv_file << "Dataset: Trajectory (100k points)\n";
    csv_file << "Comparison: Serf-QT vs CoST\n";
    
    // (comment removed)
    TestErrorThreshold(full_data, csv_file);
    TestSamplingInterval(csv_file);  //  full_data
    TestWindowSize(full_data, csv_file);
    
    csv_file.close();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "！" << std::endl;
    std::cout << ": " << csv_filename.str() << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
