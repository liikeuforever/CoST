/**
 * TrajCompress-SP 
 * 
 * ：
 * 1. Geolife GPS
 * 2. TrajCompress-SP
 * 3. 
 * 4. Serf-QT
 * 5. 
 */

#include "baselines/trajcompress/trajcompress_sp_compressor.h"
#include "baselines/trajcompress/trajcompress_sp_adaptive_compressor.h"
#include "algorithm/cost_compressor.h"
#include "baselines/serf/serf_qt_compressor.h"
#include "baselines/serf/serf_qt_linear_compressor.h"
#include "baselines/serf/serf_qt_curve_compressor.h"
#include "baselines/serf/serf_qt_curve_decompressor.h"
#include "baselines/serf/serf_qt_decompressor.h"
#include "baselines/serf/serf_qt_linear_decompressor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <chrono>
#include <iomanip>

using GpsPoint = TrajCompressSPCompressor::GpsPoint;
using AdaptiveGpsPoint = TrajCompressSPAdaptiveCompressor::GpsPoint;
using CoSTGpsPoint = CoSTCompressor::GpsPoint;

// (comment removed)
inline GpsPoint ToGpsPoint(const CoSTGpsPoint& p) {
    return GpsPoint(p.longitude, p.latitude, p.timestamp);
}

inline CoSTGpsPoint ToCoSTGpsPoint(const GpsPoint& p) {
    return CoSTGpsPoint(p.longitude, p.latitude, p.timestamp);
}

// (comment removed)
inline std::vector<GpsPoint> ToGpsPointVector(const std::vector<CoSTGpsPoint>& points) {
    std::vector<GpsPoint> result;
    result.reserve(points.size());
    for (const auto& p : points) {
        result.push_back(ToGpsPoint(p));
    }
    return result;
}

inline std::vector<CoSTGpsPoint> ToCoSTGpsPointVector(const std::vector<GpsPoint>& points) {
    std::vector<CoSTGpsPoint> result;
    result.reserve(points.size());
    for (const auto& p : points) {
        result.push_back(ToCoSTGpsPoint(p));
    }
    return result;
}

// ：Unix（）
uint64_t ParseTimestamp(const std::string& ts_str) {
 // : "YYYY-MM-DD HH:MM:SS"
    std::tm tm = {};
    std::istringstream ss(ts_str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    
    if (ss.fail()) {
        return 0;  // 0
    }
    
    return static_cast<uint64_t>(std::mktime(&tm));
}

// CSVGPS（）
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
                
 // （）
                uint64_t timestamp = 0;
                if (std::getline(ss, ts_str)) {
                    timestamp = ParseTimestamp(ts_str);
                }
                
                points.emplace_back(longitude, latitude, timestamp);
                count++;
            } catch (const std::exception& e) {
                std::cerr << ": " << line << std::endl;
            }
        }
    }
    
    file.close();
    std::cout << " " << points.size() << " GPS";
    if (!points.empty() && points[0].timestamp > 0) {
        std::cout << "（）";
    }
    std::cout << std::endl;
    return points;
}

// （）- ，
double CalculateDistance(const CoSTGpsPoint& p1, const CoSTGpsPoint& p2) {
    double dx = p1.longitude - p2.longitude;
    double dy = p1.latitude - p2.latitude;
    return std::sqrt(dx * dx + dy * dy);
}

// ：GpsPoint
double CalculateDistance(const GpsPoint& p1, const GpsPoint& p2) {
    double dx = p1.longitude - p2.longitude;
    double dy = p1.latitude - p2.latitude;
    return std::sqrt(dx * dx + dy * dy);
}

// (comment removed)
void CalculateErrors(const std::vector<CoSTGpsPoint>& original,
                    const std::vector<CoSTGpsPoint>& decompressed,
                    double& max_error,
                    double& avg_error,
                    int& points_exceeding_threshold,
                    double threshold) {
    max_error = 0;
    avg_error = 0;
    points_exceeding_threshold = 0;
    
    if (original.size() != decompressed.size()) {
        std::cerr << ":  (" 
                  << original.size() << " vs " << decompressed.size() << ")" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < original.size(); ++i) {
        double error = CalculateDistance(original[i], decompressed[i]);
        max_error = std::max(max_error, error);
        avg_error += error;
        
        if (error > threshold) {
            points_exceeding_threshold++;
        }
    }
    
    avg_error /= original.size();
}

// ：GpsPointCoSTGpsPoint
void CalculateErrors(const std::vector<CoSTGpsPoint>& original,
                    const std::vector<GpsPoint>& decompressed,
                    double& max_error,
                    double& avg_error,
                    int& points_exceeding_threshold,
                    double threshold) {
    max_error = 0;
    avg_error = 0;
    points_exceeding_threshold = 0;
    
    if (original.size() != decompressed.size()) {
        std::cerr << ":  (" 
                  << original.size() << " vs " << decompressed.size() << ")" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < original.size(); ++i) {
        double error = CalculateDistance(ToGpsPoint(original[i]), decompressed[i]);
        max_error = std::max(max_error, error);
        avg_error += error;
        
        if (error > threshold) {
            points_exceeding_threshold++;
        }
    }
    
    avg_error /= original.size();
}

// ：GpsPoint vs GpsPoint
void CalculateErrors(const std::vector<GpsPoint>& original,
                    const std::vector<GpsPoint>& decompressed,
                    double& max_error,
                    double& avg_error,
                    int& points_exceeding_threshold,
                    double threshold) {
    max_error = 0;
    avg_error = 0;
    points_exceeding_threshold = 0;
    
    if (original.size() != decompressed.size()) {
        std::cerr << ":  (" 
                  << original.size() << " vs " << decompressed.size() << ")" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < original.size(); ++i) {
        double error = CalculateDistance(original[i], decompressed[i]);
        max_error = std::max(max_error, error);
        avg_error += error;
        
        if (error > threshold) {
            points_exceeding_threshold++;
        }
    }
    
    avg_error /= original.size();
}

// TrajCompress-SP
void TestTrajCompressSP(const std::vector<GpsPoint>& gps_data, double epsilon) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << " TrajCompress-SP " << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << ": " << gps_data.size() << std::endl;
    std::cout << ": " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    
    // (comment removed)
    auto start_time = std::chrono::high_resolution_clock::now();
    
    TrajCompressSPCompressor compressor(gps_data.size(), epsilon);
    
    for (const auto& point : gps_data) {
        compressor.AddGpsPoint(point);
    }
    
    compressor.Close();
    
    auto end_compress_time = std::chrono::high_resolution_clock::now();
    auto compress_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_compress_time - start_time).count();
    
    // (comment removed)
    Array<uint8_t> compressed_data = compressor.GetCompressedData();
    int compressed_size_bits = compressor.GetCompressedSizeInBits();
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << compress_duration << " ms" << std::endl;
    std::cout << ": " << compressed_size_bits << " bits (" 
              << (compressed_size_bits / 8) << " bytes)" << std::endl;
    
    // (comment removed)
    auto start_decompress_time = std::chrono::high_resolution_clock::now();
    
    TrajCompressSPDecompressor decompressor(compressed_data.begin(), compressed_data.length());
    std::vector<GpsPoint> decompressed_data;
    GpsPoint point;
    for (size_t i = 0; i < gps_data.size(); ++i) {
        if (decompressor.ReadNextPoint(point)) {
            decompressed_data.push_back(point);
        } else {
            break;
        }
    }
    
    auto end_decompress_time = std::chrono::high_resolution_clock::now();
    auto decompress_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_decompress_time - start_decompress_time).count();
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << decompress_duration << " ms" << std::endl;
    std::cout << ": " << decompressed_data.size() << std::endl;
    
    // (comment removed)
    double max_error, avg_error;
    int points_exceeding;
    CalculateErrors(gps_data, decompressed_data, max_error, avg_error, 
                   points_exceeding, epsilon);
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << std::scientific << std::setprecision(6) << max_error 
              << "  (" << std::fixed << std::setprecision(2) << (max_error * 111000) << " )" << std::endl;
    std::cout << ": " << std::scientific << avg_error 
              << "  (" << std::fixed << (avg_error * 111000) << " )" << std::endl;
    std::cout << ": " << points_exceeding << " / " << gps_data.size() 
              << " (" << std::setprecision(2) << (100.0 * points_exceeding / gps_data.size()) << "%)" << std::endl;
    
 // - 
    std::cout << "\n---  ---" << std::endl;
    bool precision_satisfied = (max_error <= epsilon);
    std::cout << ": " << std::scientific << epsilon << "  (" 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    std::cout << ": " << std::scientific << max_error << "  (" 
              << std::fixed << (max_error * 111000) << " )" << std::endl;
    
    // (comment removed)
    if (precision_satisfied) {
        std::cout << "✅ ！ " << std::scientific << max_error 
                  << " ≤  " << epsilon << std::endl;
    } else {
        std::cout << "❌ ！ " << std::scientific << max_error 
                  << " >  " << epsilon << std::endl;
        std::cout << "⚠️  ：1e-5，" << std::endl;
    }
    
    // (comment removed)
    std::cout << "\n---  ---" << std::endl;
    std::vector<double> errors;
    for (size_t i = 0; i < gps_data.size() && i < decompressed_data.size(); ++i) {
        double error = CalculateDistance(gps_data[i], decompressed_data[i]);
        errors.push_back(error);
    }
    
    if (!errors.empty()) {
        std::sort(errors.begin(), errors.end());
        
        size_t p50_idx = errors.size() * 0.50;
        size_t p90_idx = errors.size() * 0.90;
        size_t p95_idx = errors.size() * 0.95;
        size_t p99_idx = errors.size() * 0.99;
        
        std::cout << "P50 (): " << std::scientific << errors[p50_idx] 
                  << "  (" << std::fixed << std::setprecision(2) 
                  << (errors[p50_idx] * 111000) << " )" << std::endl;
        std::cout << "P90: " << std::scientific << errors[p90_idx] 
                  << "  (" << std::fixed << (errors[p90_idx] * 111000) << " )" << std::endl;
        std::cout << "P95: " << std::scientific << errors[p95_idx] 
                  << "  (" << std::fixed << (errors[p95_idx] * 111000) << " )" << std::endl;
        std::cout << "P99: " << std::scientific << errors[p99_idx] 
                  << "  (" << std::fixed << (errors[p99_idx] * 111000) << " )" << std::endl;
        
        // (comment removed)
        int within_epsilon = 0;
        int within_2epsilon = 0;
        int within_5epsilon = 0;
        
        for (double error : errors) {
            if (error <= epsilon) within_epsilon++;
            if (error <= 2 * epsilon) within_2epsilon++;
            if (error <= 5 * epsilon) within_5epsilon++;
        }
        
        std::cout << "\n:" << std::endl;
        std::cout << "≤ 1×: " << within_epsilon << " / " << errors.size() 
                  << " (" << std::setprecision(1) << (100.0 * within_epsilon / errors.size()) << "%)" << std::endl;
        std::cout << "≤ 2×: " << within_2epsilon << " / " << errors.size() 
                  << " (" << (100.0 * within_2epsilon / errors.size()) << "%)" << std::endl;
        std::cout << "≤ 5×: " << within_5epsilon << " / " << errors.size() 
                  << " (" << (100.0 * within_5epsilon / errors.size()) << "%)" << std::endl;
    }
    
    // (comment removed)
    compressor.GetStats().PrintDetailedStats();
}

// Serf-QT（）
void TestSerfQT(const std::vector<GpsPoint>& gps_data, double epsilon) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << " Serf-QT （）" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << ": " << gps_data.size() << std::endl;
    std::cout << ": " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    
 // （，）
    auto start_time = std::chrono::high_resolution_clock::now();
    
    SerfQtCompressor lon_compressor(gps_data.size(), epsilon);
    SerfQtCompressor lat_compressor(gps_data.size(), epsilon);
    
    for (const auto& point : gps_data) {
        lon_compressor.AddValue(point.longitude);
        lat_compressor.AddValue(point.latitude);
    }
    
    lon_compressor.Close();
    lat_compressor.Close();
    
    auto end_compress_time = std::chrono::high_resolution_clock::now();
    auto compress_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_compress_time - start_time).count();
    
    int total_bits = lon_compressor.get_compressed_size_in_bits() + 
                     lat_compressor.get_compressed_size_in_bits();
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << compress_duration << " ms" << std::endl;
    std::cout << ": " << total_bits << " bits (" << (total_bits / 8) << " bytes)" << std::endl;
    std::cout << "  : " << lon_compressor.get_compressed_size_in_bits() << " bits" << std::endl;
    std::cout << "  : " << lat_compressor.get_compressed_size_in_bits() << " bits" << std::endl;
    
    // (comment removed)
    int original_bits = gps_data.size() * 128;  // 2double = 128 bits
    double compression_ratio = static_cast<double>(original_bits) / total_bits;
    double avg_bits_per_point = static_cast<double>(total_bits) / gps_data.size();
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << original_bits << " bits" << std::endl;
    std::cout << ": " << std::fixed << std::setprecision(2) << compression_ratio << ":1" << std::endl;
    std::cout << ": " << avg_bits_per_point << " bits/" << std::endl;
}

// Serf-QT-Linear（）
void TestSerfQTLinear(const std::vector<GpsPoint>& gps_data, double epsilon) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << " Serf-QT-Linear （）" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << ": " << gps_data.size() << std::endl;
    std::cout << ": " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    
    // (comment removed)
    auto start_time = std::chrono::high_resolution_clock::now();
    
    SerfQtLinearCompressor lon_compressor(gps_data.size(), epsilon);
    SerfQtLinearCompressor lat_compressor(gps_data.size(), epsilon);
    
    for (const auto& point : gps_data) {
        lon_compressor.AddValue(point.longitude, point.timestamp);
        lat_compressor.AddValue(point.latitude, point.timestamp);
    }
    
    lon_compressor.Close();
    lat_compressor.Close();
    
    auto end_compress_time = std::chrono::high_resolution_clock::now();
    auto compress_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_compress_time - start_time).count();
    
    int total_bits = lon_compressor.get_compressed_size_in_bits() + 
                     lat_compressor.get_compressed_size_in_bits();
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << compress_duration << " ms" << std::endl;
    std::cout << ": " << total_bits << " bits (" << (total_bits / 8) << " bytes)" << std::endl;
    std::cout << "  : " << lon_compressor.get_compressed_size_in_bits() << " bits" << std::endl;
    std::cout << "  : " << lat_compressor.get_compressed_size_in_bits() << " bits" << std::endl;
    
    // (comment removed)
    int original_bits = gps_data.size() * 128;
    double compression_ratio = static_cast<double>(original_bits) / total_bits;
    double avg_bits_per_point = static_cast<double>(total_bits) / gps_data.size();
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << original_bits << " bits" << std::endl;
    std::cout << ": " << std::fixed << std::setprecision(2) << compression_ratio << ":1" << std::endl;
    std::cout << ": " << avg_bits_per_point << " bits/" << std::endl;
}

// Serf-QT-Curve（）
void TestSerfQTCurve(const std::vector<GpsPoint>& gps_data, double epsilon) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << " Serf-QT-Curve （）" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << ": " << gps_data.size() << std::endl;
    std::cout << ": " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    
    // (comment removed)
    auto start_time = std::chrono::high_resolution_clock::now();
    
    SerfQtCurveCompressor lon_compressor(gps_data.size(), epsilon);
    SerfQtCurveCompressor lat_compressor(gps_data.size(), epsilon);
    
    for (const auto& point : gps_data) {
        lon_compressor.AddValue(point.longitude, point.timestamp);
        lat_compressor.AddValue(point.latitude, point.timestamp);
    }
    
    lon_compressor.Close();
    lat_compressor.Close();
    
    auto end_compress_time = std::chrono::high_resolution_clock::now();
    auto compress_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_compress_time - start_time).count();
    
    int total_bits = lon_compressor.get_compressed_size_in_bits() + 
                     lat_compressor.get_compressed_size_in_bits();
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << compress_duration << " ms" << std::endl;
    std::cout << ": " << total_bits << " bits (" << (total_bits / 8) << " bytes)" << std::endl;
    std::cout << "  : " << lon_compressor.get_compressed_size_in_bits() << " bits" << std::endl;
    std::cout << "  : " << lat_compressor.get_compressed_size_in_bits() << " bits" << std::endl;
    
    // (comment removed)
    int original_bits = gps_data.size() * 128;
    double compression_ratio = static_cast<double>(original_bits) / total_bits;
    double avg_bits_per_point = static_cast<double>(total_bits) / gps_data.size();
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << original_bits << " bits" << std::endl;
    std::cout << ": " << std::fixed << std::setprecision(2) << compression_ratio << ":1" << std::endl;
    std::cout << ": " << avg_bits_per_point << " bits/" << std::endl;
}

// TrajCompress-SP-Adaptive（）- 
/*
void TestTrajCompressSPAdaptive(const std::vector<GpsPoint>& gps_data, double epsilon) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << " TrajCompress-SP-Adaptive （）" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << ": " << gps_data.size() << std::endl;
    std::cout << ": " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    
 // （）
    auto start_time = std::chrono::high_resolution_clock::now();
    
    TrajCompressSPAdaptiveCompressor compressor(
        gps_data.size(), 
        epsilon,
        true,  // enable_adaptive = true（）
        16,    // min_window（）
        192,   // max_window（）
        256    // observe_window
    );
    
    for (const auto& point : gps_data) {
        compressor.AddGpsPoint(AdaptiveGpsPoint(point.longitude, point.latitude));
    }
    
    compressor.Close();
    
    auto end_compress_time = std::chrono::high_resolution_clock::now();
    auto compress_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_compress_time - start_time).count();
    
    // (comment removed)
    Array<uint8_t> compressed_data = compressor.GetCompressedData();
    int compressed_size_bits = compressor.GetCompressedSizeInBits();
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << compress_duration << " ms" << std::endl;
    std::cout << ": " << compressed_size_bits << " bits (" 
              << (compressed_size_bits / 8) << " bytes)" << std::endl;
    
    // (comment removed)
    int original_bits = gps_data.size() * 128;  // 2double = 128 bits
    double compression_ratio = static_cast<double>(original_bits) / compressed_size_bits;
    double avg_bits_per_point = static_cast<double>(compressed_size_bits) / gps_data.size();
    
    std::cout << "\n---  ---" << std::endl;
    std::cout << ": " << original_bits << " bits" << std::endl;
    std::cout << ": " << std::fixed << std::setprecision(2) << compression_ratio << ":1" << std::endl;
    std::cout << ": " << avg_bits_per_point << " bits/" << std::endl;
    
    // (comment removed)
    compressor.GetStats().PrintDetailedStats();
}
*/

// (comment removed)
void FiveWayComparativeTest(const std::vector<GpsPoint>& gps_data, double epsilon) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << ": " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    std::cout << ": 2 * epsilon * 0.999" << std::endl;
    std::cout << ": " << gps_data.size() << std::endl;
    
 // TrajCompress-SP ()
    TrajCompressSPCompressor sp_compressor(gps_data.size(), epsilon);
    for (const auto& point : gps_data) {
        sp_compressor.AddGpsPoint(point);
    }
    sp_compressor.Close();
 // ，timestamp bits（）
    auto sp_stats_2 = sp_compressor.GetStats();
    int sp_spatial_bits_2 = sp_stats_2.total_bits - sp_stats_2.timestamp_bits;
    double sp_avg_bits = static_cast<double>(sp_spatial_bits_2) / gps_data.size();
    
 // TrajCompress-SP-Adaptive ()
    /*
    TrajCompressSPAdaptiveCompressor adaptive_compressor(
        gps_data.size(), 
        epsilon,
        true,  // enable_adaptive = true（）
        16,    // min_window（）
        192,   // max_window（）
        256    // observe_window
    );
    for (const auto& point : gps_data) {
        adaptive_compressor.AddGpsPoint(AdaptiveGpsPoint(point.longitude, point.latitude));
    }
    adaptive_compressor.Close();
    */
    int adaptive_bits = sp_spatial_bits_2;  // SP
    double adaptive_avg_bits = static_cast<double>(adaptive_bits) / gps_data.size();
    
 // Serf-QT (/)
    SerfQtCompressor qt_lon(gps_data.size(), epsilon);
    SerfQtCompressor qt_lat(gps_data.size(), epsilon);
    for (const auto& point : gps_data) {
        qt_lon.AddValue(point.longitude);
        qt_lat.AddValue(point.latitude);
    }
    qt_lon.Close();
    qt_lat.Close();
    int qt_bits = qt_lon.get_compressed_size_in_bits() + qt_lat.get_compressed_size_in_bits();
    double qt_avg_bits = static_cast<double>(qt_bits) / gps_data.size();
    
 // Serf-QT-Linear ()
    SerfQtLinearCompressor linear_lon(gps_data.size(), epsilon);
    SerfQtLinearCompressor linear_lat(gps_data.size(), epsilon);
    for (const auto& point : gps_data) {
        linear_lon.AddValue(point.longitude);
        linear_lat.AddValue(point.latitude);
    }
    linear_lon.Close();
    linear_lat.Close();
    int linear_bits = linear_lon.get_compressed_size_in_bits() + linear_lat.get_compressed_size_in_bits();
    double linear_avg_bits = static_cast<double>(linear_bits) / gps_data.size();
    
 // Serf-QT-Curve ()
    SerfQtCurveCompressor curve_lon(gps_data.size(), epsilon);
    SerfQtCurveCompressor curve_lat(gps_data.size(), epsilon);
    for (const auto& point : gps_data) {
        curve_lon.AddValue(point.longitude);
        curve_lat.AddValue(point.latitude);
    }
    curve_lon.Close();
    curve_lat.Close();
    int curve_bits = curve_lon.get_compressed_size_in_bits() + curve_lat.get_compressed_size_in_bits();
    double curve_avg_bits = static_cast<double>(curve_bits) / gps_data.size();
    
    // (comment removed)
    std::cout << "\n:" << std::endl;
    std::cout << std::string(120, '-') << std::endl;
    std::cout << std::setw(30) << "" 
              << std::setw(20) << "(bits)" 
              << std::setw(20) << "(bits)"
              << std::setw(20) << ""
              << std::setw(30) << "/Serf-QT" << std::endl;
    std::cout << std::string(120, '-') << std::endl;
    
    int original_bits = gps_data.size() * 128;
    int min_bits = std::min({sp_spatial_bits_2, adaptive_bits, qt_bits, linear_bits, curve_bits});
    
    // TrajCompress-SP
    std::cout << std::setw(30) << "TrajCompress-SP" 
              << std::setw(20) << sp_spatial_bits_2
              << std::setw(20) << std::fixed << std::setprecision(2) << sp_avg_bits
              << std::setw(20) << (static_cast<double>(original_bits) / sp_spatial_bits_2) << ":1"
              << std::setw(15) << std::setprecision(1) 
              << ((1.0 - static_cast<double>(sp_spatial_bits_2) / min_bits) * 100) << "%"
              << std::setw(15)
              << ((1.0 - static_cast<double>(sp_spatial_bits_2) / qt_bits) * 100) << "%" << std::endl;
    
    // TrajCompress-SP-Adaptive
    std::cout << std::setw(30) << "TrajCompress-SP-Adaptive" 
              << std::setw(20) << adaptive_bits
              << std::setw(20) << std::setprecision(2) << adaptive_avg_bits
              << std::setw(20) << (static_cast<double>(original_bits) / adaptive_bits) << ":1"
              << std::setw(15) << std::setprecision(1) 
              << ((1.0 - static_cast<double>(adaptive_bits) / min_bits) * 100) << "%"
              << std::setw(15)
              << ((1.0 - static_cast<double>(adaptive_bits) / qt_bits) * 100) << "%" << std::endl;
    
 // Serf-QT ()
    std::cout << std::setw(30) << "Serf-QT ()" 
              << std::setw(20) << qt_bits
              << std::setw(20) << std::setprecision(2) << qt_avg_bits
              << std::setw(20) << (static_cast<double>(original_bits) / qt_bits) << ":1"
              << std::setw(15) << std::setprecision(1) 
              << ((1.0 - static_cast<double>(qt_bits) / min_bits) * 100) << "%"
              << std::setw(15) << "0.0%" << std::endl;
    
    // Serf-QT-Linear
    std::cout << std::setw(30) << "Serf-QT-Linear" 
              << std::setw(20) << linear_bits
              << std::setw(20) << std::setprecision(2) << linear_avg_bits
              << std::setw(20) << (static_cast<double>(original_bits) / linear_bits) << ":1"
              << std::setw(15) << std::setprecision(1) 
              << ((1.0 - static_cast<double>(linear_bits) / min_bits) * 100) << "%"
              << std::setw(15)
              << ((1.0 - static_cast<double>(linear_bits) / qt_bits) * 100) << "%" << std::endl;
    
    // Serf-QT-Curve
    std::cout << std::setw(30) << "Serf-QT-Curve" 
              << std::setw(20) << curve_bits
              << std::setw(20) << std::setprecision(2) << curve_avg_bits
              << std::setw(20) << (static_cast<double>(original_bits) / curve_bits) << ":1"
              << std::setw(15) << std::setprecision(1) 
              << ((1.0 - static_cast<double>(curve_bits) / min_bits) * 100) << "%"
              << std::setw(15)
              << ((1.0 - static_cast<double>(curve_bits) / qt_bits) * 100) << "%" << std::endl;
    
    std::cout << std::string(120, '-') << std::endl;
    
    // (comment removed)
    std::string best_algorithm;
    if (sp_spatial_bits_2 == min_bits) best_algorithm = "TrajCompress-SP";
    else if (adaptive_bits == min_bits) best_algorithm = "TrajCompress-SP-Adaptive";
    else if (qt_bits == min_bits) best_algorithm = "Serf-QT ()";
    else if (linear_bits == min_bits) best_algorithm = "Serf-QT-Linear";
    else best_algorithm = "Serf-QT-Curve";
    
    std::cout << "\n✅ : " << best_algorithm << std::endl;
    std::cout << "   : " << min_bits << " bits (" << std::setprecision(2) 
              << (static_cast<double>(min_bits) / gps_data.size()) << " bits/)" << std::endl;
    
 // Serf-QT
    std::cout << "\nSerf-QT () :" << std::endl;
    std::cout << "  TrajCompress-SP:          " << std::fixed << std::setprecision(1)
              << ((1.0 - static_cast<double>(sp_spatial_bits_2) / qt_bits) * 100) << "%" << std::endl;
    std::cout << "  TrajCompress-SP-Adaptive: "
              << ((1.0 - static_cast<double>(adaptive_bits) / qt_bits) * 100) << "%" << std::endl;
    std::cout << "  Serf-QT-Linear:           "
              << ((1.0 - static_cast<double>(linear_bits) / qt_bits) * 100) << "%" << std::endl;
    std::cout << "  Serf-QT-Curve:            "
              << ((1.0 - static_cast<double>(curve_bits) / qt_bits) * 100) << "%" << std::endl;
    
    // (comment removed)
    std::cout << "\n:" << std::endl;
    std::cout << "  - TrajCompress-SP: (LDR/CP/ZP)，" << std::endl;
    std::cout << "  - TrajCompress-SP-Adaptive:  + (Multi/LDR-Only)" << std::endl;
    std::cout << "  - Serf-QT (): ，" << std::endl;
    std::cout << "  - Serf-QT-Linear: ，" << std::endl;
    std::cout << "  - Serf-QT-Curve: ，//" << std::endl;
}

// （）
void FourWayComparativeTest(const std::vector<GpsPoint>& gps_data, double epsilon) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << ": " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    std::cout << ": 2 * epsilon * 0.999" << std::endl;
    std::cout << ": " << gps_data.size() << std::endl;
    
 // TrajCompress-SP ()
    TrajCompressSPCompressor sp_compressor(gps_data.size(), epsilon);
    for (const auto& point : gps_data) {
        sp_compressor.AddGpsPoint(point);
    }
    sp_compressor.Close();
 // ，timestamp bits（）
    auto sp_stats_3 = sp_compressor.GetStats();
    int sp_spatial_bits_3 = sp_stats_3.total_bits - sp_stats_3.timestamp_bits;
    double sp_avg_bits = static_cast<double>(sp_spatial_bits_3) / gps_data.size();
    
 // Serf-QT (/)
    SerfQtCompressor qt_lon(gps_data.size(), epsilon);
    SerfQtCompressor qt_lat(gps_data.size(), epsilon);
    for (const auto& point : gps_data) {
        qt_lon.AddValue(point.longitude);
        qt_lat.AddValue(point.latitude);
    }
    qt_lon.Close();
    qt_lat.Close();
    int qt_bits = qt_lon.get_compressed_size_in_bits() + qt_lat.get_compressed_size_in_bits();
    double qt_avg_bits = static_cast<double>(qt_bits) / gps_data.size();
    
 // Serf-QT-Linear ()
    SerfQtLinearCompressor linear_lon(gps_data.size(), epsilon);
    SerfQtLinearCompressor linear_lat(gps_data.size(), epsilon);
    for (const auto& point : gps_data) {
        linear_lon.AddValue(point.longitude);
        linear_lat.AddValue(point.latitude);
    }
    linear_lon.Close();
    linear_lat.Close();
    int linear_bits = linear_lon.get_compressed_size_in_bits() + linear_lat.get_compressed_size_in_bits();
    double linear_avg_bits = static_cast<double>(linear_bits) / gps_data.size();
    
 // Serf-QT-Curve ()
    SerfQtCurveCompressor curve_lon(gps_data.size(), epsilon);
    SerfQtCurveCompressor curve_lat(gps_data.size(), epsilon);
    for (const auto& point : gps_data) {
        curve_lon.AddValue(point.longitude);
        curve_lat.AddValue(point.latitude);
    }
    curve_lon.Close();
    curve_lat.Close();
    int curve_bits = curve_lon.get_compressed_size_in_bits() + curve_lat.get_compressed_size_in_bits();
    double curve_avg_bits = static_cast<double>(curve_bits) / gps_data.size();
    
    // (comment removed)
    std::cout << "\n:" << std::endl;
    std::cout << std::string(100, '-') << std::endl;
    std::cout << std::setw(25) << "" 
              << std::setw(20) << "(bits)" 
              << std::setw(20) << "(bits)"
              << std::setw(20) << ""
              << std::setw(15) << "" << std::endl;
    std::cout << std::string(100, '-') << std::endl;
    
    int original_bits = gps_data.size() * 128;
    int min_bits = std::min({sp_spatial_bits_3, qt_bits, linear_bits, curve_bits});
    
    // TrajCompress-SP
    std::cout << std::setw(25) << "TrajCompress-SP" 
              << std::setw(20) << sp_spatial_bits_3
              << std::setw(20) << std::fixed << std::setprecision(2) << sp_avg_bits
              << std::setw(20) << (static_cast<double>(original_bits) / sp_spatial_bits_3) << ":1"
              << std::setw(15) << std::setprecision(1) 
              << ((1.0 - static_cast<double>(sp_spatial_bits_3) / min_bits) * 100) << "%" << std::endl;
    
 // Serf-QT ()
    std::cout << std::setw(25) << "Serf-QT ()" 
              << std::setw(20) << qt_bits
              << std::setw(20) << std::setprecision(2) << qt_avg_bits
              << std::setw(20) << (static_cast<double>(original_bits) / qt_bits) << ":1"
              << std::setw(15) << std::setprecision(1) 
              << ((1.0 - static_cast<double>(qt_bits) / min_bits) * 100) << "%" << std::endl;
    
    // Serf-QT-Linear
    std::cout << std::setw(25) << "Serf-QT-Linear" 
              << std::setw(20) << linear_bits
              << std::setw(20) << std::setprecision(2) << linear_avg_bits
              << std::setw(20) << (static_cast<double>(original_bits) / linear_bits) << ":1"
              << std::setw(15) << std::setprecision(1) 
              << ((1.0 - static_cast<double>(linear_bits) / min_bits) * 100) << "%" << std::endl;
    
    // Serf-QT-Curve
    std::cout << std::setw(25) << "Serf-QT-Curve" 
              << std::setw(20) << curve_bits
              << std::setw(20) << std::setprecision(2) << curve_avg_bits
              << std::setw(20) << (static_cast<double>(original_bits) / curve_bits) << ":1"
              << std::setw(15) << std::setprecision(1) 
              << ((1.0 - static_cast<double>(curve_bits) / min_bits) * 100) << "%" << std::endl;
    
    std::cout << std::string(100, '-') << std::endl;
    
    // (comment removed)
    std::string best_algorithm;
    if (sp_spatial_bits_3 == min_bits) best_algorithm = "TrajCompress-SP";
    else if (qt_bits == min_bits) best_algorithm = "Serf-QT ()";
    else if (linear_bits == min_bits) best_algorithm = "Serf-QT-Linear";
    else best_algorithm = "Serf-QT-Curve";
    
    std::cout << "\n✅ : " << best_algorithm << std::endl;
    std::cout << "   : " << min_bits << " bits (" << std::setprecision(2) 
              << (static_cast<double>(min_bits) / gps_data.size()) << " bits/)" << std::endl;
    
 // Serf-QT
    std::cout << "\nSerf-QT () :" << std::endl;
    std::cout << "  TrajCompress-SP:    " << std::fixed << std::setprecision(1)
              << ((1.0 - static_cast<double>(sp_spatial_bits_3) / qt_bits) * 100) << "%" << std::endl;
    std::cout << "  Serf-QT-Linear:     "
              << ((1.0 - static_cast<double>(linear_bits) / qt_bits) * 100) << "%" << std::endl;
    std::cout << "  Serf-QT-Curve:      "
              << ((1.0 - static_cast<double>(curve_bits) / qt_bits) * 100) << "%" << std::endl;
    
    // (comment removed)
    std::cout << "\n:" << std::endl;
    std::cout << "  - TrajCompress-SP: (LDR/CP/ZP)，" << std::endl;
    std::cout << "  - Serf-QT (): ，" << std::endl;
    std::cout << "  - Serf-QT-Linear: ，" << std::endl;
    std::cout << "  - Serf-QT-Curve: ，//" << std::endl;
}

    // (comment removed)
void ComparativeTest(const std::vector<GpsPoint>& gps_data, double epsilon) {
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "TrajCompress-SP vs Serf-QT " << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "TrajCompress-SP: " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    std::cout << "Serf-QT: " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    std::cout << "： (2 * epsilon * 0.999)" << std::endl;
    
 // TrajCompress-SP（Serf-QT）
    TrajCompressSPCompressor sp_compressor(gps_data.size(), epsilon);
    for (const auto& point : gps_data) {
        sp_compressor.AddGpsPoint(point);
    }
    sp_compressor.Close();
    
 // ，timestamp bits（）
    auto sp_stats = sp_compressor.GetStats();
    int sp_spatial_bits = sp_stats.total_bits - sp_stats.timestamp_bits;
    double sp_avg_bits = static_cast<double>(sp_spatial_bits) / gps_data.size();
    
 // Serf-QT (，epsilon)
    SerfQtCompressor qt_lon(gps_data.size(), epsilon);
    SerfQtCompressor qt_lat(gps_data.size(), epsilon);
    for (const auto& point : gps_data) {
        qt_lon.AddValue(point.longitude);
        qt_lat.AddValue(point.latitude);
    }
    qt_lon.Close();
    qt_lat.Close();
    
    int qt_bits = qt_lon.get_compressed_size_in_bits() + qt_lat.get_compressed_size_in_bits();
    double qt_avg_bits = static_cast<double>(qt_bits) / gps_data.size();
    
    // (comment removed)
    std::cout << "\n:" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    std::cout << std::setw(25) << "" << std::setw(20) << "TrajCompress-SP" 
              << std::setw(20) << "Serf-QT" << std::setw(15) << "" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    std::cout << std::setw(25) << " (bits)" 
              << std::setw(20) << sp_spatial_bits 
              << std::setw(20) << qt_bits
              << std::setw(15) << std::fixed << std::setprecision(1) 
              << ((1.0 - static_cast<double>(sp_spatial_bits) / qt_bits) * 100) << "%" << std::endl;
    
    std::cout << std::setw(25) << " (bits)" 
              << std::setw(20) << std::setprecision(2) << sp_avg_bits 
              << std::setw(20) << qt_avg_bits
              << std::setw(15) << std::setprecision(1) 
              << ((1.0 - sp_avg_bits / qt_avg_bits) * 100) << "%" << std::endl;
    
    int original_bits = gps_data.size() * 128;
    double sp_ratio = static_cast<double>(original_bits) / sp_spatial_bits;
    double qt_ratio = static_cast<double>(original_bits) / qt_bits;
    
    std::cout << std::setw(25) << "" 
              << std::setw(20) << std::setprecision(2) << sp_ratio << ":1"
              << std::setw(20) << qt_ratio << ":1"
              << std::setw(15) << std::setprecision(1) 
              << ((sp_ratio - qt_ratio) / qt_ratio * 100) << "%" << std::endl;
    
    std::cout << std::string(80, '-') << std::endl;
    
    if (sp_spatial_bits < qt_bits) {
        double improvement = (1.0 - static_cast<double>(sp_spatial_bits) / qt_bits) * 100;
        std::cout << "\n✅ TrajCompress-SP  Serf-QT : " 
                  << std::fixed << std::setprecision(1) << improvement << "%" << std::endl;
    } else {
        std::cout << "\n⚠️  TrajCompress-SP  Serf-QT" << std::endl;
    }
    
    // (comment removed)
    std::cout << "\n---  ---" << std::endl;
    std::cout << "TrajCompress-SP: " << std::scientific << epsilon 
              << "  ( " << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    std::cout << "Serf-QT: " << std::scientific << epsilon 
              << "  ( " << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    std::cout << "：" << std::endl;
    std::cout << "  - TrajCompress-SP，Serf-QT" << std::endl;
    std::cout << "  - Serf-QT， 2*epsilon*0.999" << std::endl;
    std::cout << "  - " << std::endl;
}

// (comment removed)
struct DatasetConfig {
    std::string name;
    std::string path;
    int total_points;
    
    DatasetConfig(const std::string& n, const std::string& p, int pts)
        : name(n), path(p), total_points(pts) {}
};

// Get all test datasets (with timestamp)
std::vector<DatasetConfig> GetAllDatasets() {
    std::string base_path = "../../data/";
    std::vector<DatasetConfig> datasets;
    
 // Geolife （ID）
    datasets.emplace_back("Geolife ()", 
                         base_path + "Geolife_100k_with_id.csv", 100000);
    datasets.emplace_back("Geolife (5x)", 
                         base_path + "Geolife_100k_with_id_downsample_5x.csv", 20174);
    datasets.emplace_back("Geolife (10x)", 
                         base_path + "Geolife_100k_with_id_downsample_10x.csv", 10207);
    datasets.emplace_back("Geolife (20x)", 
                         base_path + "Geolife_100k_with_id_downsample_20x.csv", 5216);
    datasets.emplace_back("Geolife (40x)", 
                         base_path + "Geolife_100k_with_id_downsample_40x.csv", 2727);
    
 // Trajectory （ID）
    datasets.emplace_back("Trajectory ()", 
                         base_path + "Trajtory_100k_with_id.csv", 100000);
    datasets.emplace_back("Trajectory (5x)", 
                         base_path + "Trajtory_100k_with_id_downsample_5x.csv", 26206);
    datasets.emplace_back("Trajectory (10x)", 
                         base_path + "Trajtory_100k_with_id_downsample_10x.csv", 17096);
    datasets.emplace_back("Trajectory (20x)", 
                         base_path + "Trajtory_100k_with_id_downsample_20x.csv", 12668);
    datasets.emplace_back("Trajectory (40x)", 
                         base_path + "Trajtory_100k_with_id_downsample_40x.csv", 10561);
    
 // WX_taxi （ID）- 
    datasets.emplace_back("WX_taxi ()", 
                         base_path + "WX_taxi_100k_with_id.csv", 100000);
    datasets.emplace_back("WX_taxi (5x)", 
                         base_path + "WX_taxi_100k_with_id_downsample_5x.csv", 20188);
    datasets.emplace_back("WX_taxi (10x)", 
                         base_path + "WX_taxi_100k_with_id_downsample_10x.csv", 10209);
    datasets.emplace_back("WX_taxi (20x)", 
                         base_path + "WX_taxi_100k_with_id_downsample_20x.csv", 5218);
    datasets.emplace_back("WX_taxi (40x)", 
                         base_path + "WX_taxi_100k_with_id_downsample_40x.csv", 2725);
    
    return datasets;
}

// (comment removed)
void TestSingleDataset(const DatasetConfig& dataset, double epsilon, int max_points = -1) {
    std::cout << "\n" << std::string(100, '=') << std::endl;
    std::cout << ": " << dataset.name << std::endl;
    std::cout << std::string(100, '=') << std::endl;
    
    // (comment removed)
    auto gps_data = LoadGpsDataFromCSV(dataset.path, max_points);
    
    if (gps_data.empty()) {
        std::cerr << "GPS: " << dataset.path << std::endl;
        return;
    }
    
    std::cout << ": " << gps_data.size() << " / " << dataset.total_points << std::endl;
    std::cout << ": " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    
    // (comment removed)
    FourWayComparativeTest(ToGpsPointVector(gps_data), epsilon);
}

// (comment removed)
void TestAllDatasetsAndGenerateSummary(double epsilon) {
    auto datasets = GetAllDatasets();
    
    std::cout << "\n" << std::string(100, '=') << std::endl;
    std::cout << "" << std::endl;
    std::cout << std::string(100, '=') << std::endl;
    std::cout << ": " << datasets.size() << std::endl;
    std::cout << ": " << std::scientific << epsilon << "  ( " 
              << std::fixed << std::setprecision(2) << (epsilon * 111000) << " )" << std::endl;
    
    // (comment removed)
    struct SummaryResult {
        std::string dataset_name;
        int points;
        int sp_bits;
        int adaptive_bits;
        int simple_bits;       // Simple（）
        int simple_time_bits;  // ：Simple（）
        int qt_bits;
        int linear_bits;
        int curve_bits;
        double sp_avg;
        double adaptive_avg;
        double simple_avg;       // Simple（）
        double simple_time_avg;  // ：Simple（）
        double qt_avg;
        double linear_avg;
        double curve_avg;
 // （bits / bits，）
        double sp_cr;
        double adaptive_cr;
        double cost_cr;
        double cost_time_cr;
        double qt_cr;
        double linear_cr;
        double curve_cr;
        // (comment removed)
        double sp_max_error, sp_avg_error;
        double adaptive_max_error, adaptive_avg_error;
        double cost_max_error, cost_avg_error;
        double cost_time_max_error, simple_time_avg_error;  // ：
        double qt_max_error, qt_avg_error;
        double linear_max_error, linear_avg_error;
        double curve_max_error, curve_avg_error;
 // （/）
        double sp_compress_time, sp_decompress_time;
        double adaptive_compress_time, adaptive_decompress_time;
        double cost_compress_time, cost_decompress_time;
        double cost_time_compress_time, cost_time_decompress_time;
        double qt_compress_time, qt_decompress_time;
        double linear_compress_time, linear_decompress_time;
        double curve_compress_time, curve_decompress_time;
    };
    
    std::vector<SummaryResult> summary_results;
    
    // (comment removed)
    for (const auto& dataset : datasets) {
        std::cout << "\n" << std::string(100, '-') << std::endl;
        std::cout << ": " << dataset.name << std::endl;
        std::cout << std::string(100, '-') << std::endl;
        
        auto gps_data = LoadGpsDataFromCSV(dataset.path, -1);
        std::cout << "  [1/6] " << std::endl;
        
        if (gps_data.empty()) {
            std::cerr << ": " << dataset.name << std::endl;
            continue;
        }
        
 // TrajCompress-SP - 
        auto sp_compress_start = std::chrono::steady_clock::now();
        TrajCompressSPCompressor sp_compressor(gps_data.size(), epsilon);
        for (const auto& point : gps_data) {
            sp_compressor.AddGpsPoint(ToGpsPoint(point));
        }
        sp_compressor.Close();
        auto sp_compress_end = std::chrono::steady_clock::now();
        double sp_compress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            sp_compress_end - sp_compress_start).count() / static_cast<double>(gps_data.size());
        
 // TrajCompress-SP
        Array<uint8_t> sp_compressed = sp_compressor.GetCompressedData();
        auto sp_decompress_start = std::chrono::steady_clock::now();
        TrajCompressSPDecompressor sp_decompressor(sp_compressed.begin(), sp_compressed.length());
        std::vector<GpsPoint> sp_decompressed;
        GpsPoint sp_point;
        for (size_t i = 0; i < gps_data.size(); ++i) {
            if (sp_decompressor.ReadNextPoint(sp_point)) {
                sp_decompressed.push_back(sp_point);
            } else {
                break;
            }
        }
        auto sp_decompress_end = std::chrono::steady_clock::now();
        double sp_decompress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            sp_decompress_end - sp_decompress_start).count() / static_cast<double>(gps_data.size());
        
 // TrajCompress-SP 
        double sp_max_error, sp_avg_error;
        int sp_exceeding;
        CalculateErrors(gps_data, sp_decompressed, sp_max_error, sp_avg_error, sp_exceeding, epsilon);
        std::cout << "  [2/6] TrajCompress-SP  (max_err=" << std::scientific << sp_max_error << ")" << std::endl;
        
 // TrajCompress-SP-Adaptive（）- 
        /*
        TrajCompressSPAdaptiveCompressor adaptive_compressor(
            gps_data.size(), 
            epsilon,
            true,  // enable_adaptive = true（）
            16,    // min_window（）
            192,   // max_window（）
            256    // observe_window
        );
        for (const auto& point : gps_data) {
            adaptive_compressor.AddGpsPoint(AdaptiveGpsPoint(point.longitude, point.latitude));
        }
        adaptive_compressor.Close();
        */
        
 // TrajCompress-SP-Adaptive（）
        /*
        Array<uint8_t> adaptive_compressed = adaptive_compressor.GetCompressedData();
        TrajCompressSPAdaptiveDecompressor adaptive_decompressor(adaptive_compressed.begin(), adaptive_compressed.length());
        std::vector<GpsPoint> adaptive_decompressed;
        AdaptiveGpsPoint adaptive_point;
        for (size_t i = 0; i < gps_data.size(); ++i) {
            if (adaptive_decompressor.ReadNextPoint(adaptive_point)) {
                adaptive_decompressed.push_back(GpsPoint(adaptive_point.longitude, adaptive_point.latitude));
            } else {
                break;
            }
        }
        
        int adaptive_bits = adaptive_compressor.GetCompressedSizeInBits();
        
 // TrajCompress-SP-Adaptive 
        double adaptive_max_error, adaptive_avg_error;
        int adaptive_exceeding;
        CalculateErrors(gps_data, adaptive_decompressed, adaptive_max_error, adaptive_avg_error, adaptive_exceeding, epsilon);
        std::cout << "  [3/6] TrajCompress-SP-Adaptive  (max_err=" << std::scientific << adaptive_max_error << ", =" << adaptive_decompressed.size() << ")" << std::endl;
        */
        
 // （TrajSPbits）
        auto sp_stats_for_adaptive = sp_compressor.GetStats();
        int adaptive_bits = sp_stats_for_adaptive.total_bits - sp_stats_for_adaptive.timestamp_bits;
        double adaptive_max_error = 0.0;
        double adaptive_avg_error = 0.0;
        
 // TrajCompress-SP-Adaptive-Simple（：window_size=96，）- 
        auto cost_compress_start = std::chrono::steady_clock::now();
        CoSTCompressor cost_compressor(
            gps_data.size(), 
            epsilon,
            96,     // kEvaluationWindow = 96（）
            false   // use_time_window = false（）
        );
        for (const auto& point : gps_data) {
            cost_compressor.AddGpsPoint(CoSTGpsPoint(point.longitude, point.latitude, point.timestamp));
        }
        cost_compressor.Close();
        auto cost_compress_end = std::chrono::steady_clock::now();
        double cost_compress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            cost_compress_end - cost_compress_start).count() / static_cast<double>(gps_data.size());
        
 // TrajCompress-SP-Adaptive-Simple
        Array<uint8_t> simple_compressed = cost_compressor.GetCompressedData();
        auto cost_decompress_start = std::chrono::steady_clock::now();
        CoSTDecompressor cost_decompressor(simple_compressed.begin(), simple_compressed.length());
        std::vector<GpsPoint> simple_decompressed;
        CoSTGpsPoint simple_point;
        for (size_t i = 0; i < gps_data.size(); ++i) {
            if (cost_decompressor.ReadNextPoint(simple_point)) {
                simple_decompressed.push_back(GpsPoint(simple_point.longitude, simple_point.latitude));
            } else {
                break;
            }
        }
        auto cost_decompress_end = std::chrono::steady_clock::now();
        double cost_decompress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            cost_decompress_end - cost_decompress_start).count() / static_cast<double>(gps_data.size());
        
 // Simple 
        double cost_max_error, cost_avg_error;
        int simple_exceeding;
        CalculateErrors(gps_data, simple_decompressed, cost_max_error, cost_avg_error, simple_exceeding, epsilon);
        std::cout << "  [4/7] CoST (point-based)  (max_err=" << std::scientific << cost_max_error << ")" << std::endl;
        
 // TrajCompress-SP-Adaptive-Simple-TimeWindow（：60）- 
        auto simple_time_compress_start = std::chrono::steady_clock::now();
        CoSTCompressor simple_time_compressor(
            gps_data.size(), 
            epsilon,
            96,     // kEvaluationWindow（）
            true,   // use_time_window = true（）
            60      // time_window_seconds = 60
        );
        for (const auto& point : gps_data) {
            simple_time_compressor.AddGpsPoint(CoSTGpsPoint(point.longitude, point.latitude, point.timestamp));
        }
        simple_time_compressor.Close();
        auto simple_time_compress_end = std::chrono::steady_clock::now();
        double cost_time_compress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            simple_time_compress_end - simple_time_compress_start).count() / static_cast<double>(gps_data.size());
        
 // Simple-TimeWindow
        Array<uint8_t> simple_time_compressed = simple_time_compressor.GetCompressedData();
        auto simple_time_decompress_start = std::chrono::steady_clock::now();
        CoSTDecompressor simple_time_decompressor(simple_time_compressed.begin(), simple_time_compressed.length());
        std::vector<GpsPoint> simple_time_decompressed;
        CoSTGpsPoint simple_time_point;
        for (size_t i = 0; i < gps_data.size(); ++i) {
            if (simple_time_decompressor.ReadNextPoint(simple_time_point)) {
                simple_time_decompressed.push_back(GpsPoint(simple_time_point.longitude, simple_time_point.latitude));
            } else {
                break;
            }
        }
        auto simple_time_decompress_end = std::chrono::steady_clock::now();
        double cost_time_decompress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            simple_time_decompress_end - simple_time_decompress_start).count() / static_cast<double>(gps_data.size());
        
 // Simple-TimeWindow 
        double cost_time_max_error, simple_time_avg_error;
        int simple_time_exceeding;
        CalculateErrors(gps_data, simple_time_decompressed, cost_time_max_error, simple_time_avg_error, simple_time_exceeding, epsilon);
        std::cout << "  [5/8] CoST (time-based)  (max_err=" << std::scientific << cost_time_max_error << ")" << std::endl;
        
 // Serf-QT（timestamp）
        auto qt_compress_start = std::chrono::steady_clock::now();
        SerfQtCompressor qt_lon(gps_data.size(), epsilon);
        SerfQtCompressor qt_lat(gps_data.size(), epsilon);
        for (const auto& point : gps_data) {
            qt_lon.AddValue(point.longitude);
            qt_lat.AddValue(point.latitude);
        }
        qt_lon.Close();
        qt_lat.Close();
        auto qt_compress_end = std::chrono::steady_clock::now();
        double qt_compress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            qt_compress_end - qt_compress_start).count() / static_cast<double>(gps_data.size());
        
 // Serf-QT 
        auto qt_decompress_start = std::chrono::steady_clock::now();
        std::vector<GpsPoint> qt_decompressed;
        auto qt_lon_data = qt_lon.compressed_bytes();
        auto qt_lat_data = qt_lat.compressed_bytes();
        SerfQtDecompressor qt_lon_dec;
        SerfQtDecompressor qt_lat_dec;
        auto qt_lon_values = qt_lon_dec.Decompress(qt_lon_data);
        auto qt_lat_values = qt_lat_dec.Decompress(qt_lat_data);
        for (size_t i = 0; i < qt_lon_values.size() && i < qt_lat_values.size(); ++i) {
            qt_decompressed.emplace_back(qt_lon_values[i], qt_lat_values[i]);
        }
        auto qt_decompress_end = std::chrono::steady_clock::now();
        double qt_decompress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            qt_decompress_end - qt_decompress_start).count() / static_cast<double>(gps_data.size());
        double qt_max_error, qt_avg_error;
        int qt_exceeding;
        CalculateErrors(gps_data, qt_decompressed, qt_max_error, qt_avg_error, qt_exceeding, epsilon);
        std::cout << "  [6/8] Serf-QT  (max_err=" << std::scientific << qt_max_error << ")" << std::endl;
        
        // Serf-QT-Linear
        auto linear_compress_start = std::chrono::steady_clock::now();
        SerfQtLinearCompressor linear_lon(gps_data.size(), epsilon);
        SerfQtLinearCompressor linear_lat(gps_data.size(), epsilon);
        for (const auto& point : gps_data) {
            linear_lon.AddValue(point.longitude, point.timestamp);
            linear_lat.AddValue(point.latitude, point.timestamp);
        }
        linear_lon.Close();
        linear_lat.Close();
        auto linear_compress_end = std::chrono::steady_clock::now();
        double linear_compress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            linear_compress_end - linear_compress_start).count() / static_cast<double>(gps_data.size());
        
 // Serf-QT-Linear 
        auto linear_decompress_start = std::chrono::steady_clock::now();
        std::vector<GpsPoint> linear_decompressed;
        auto linear_lon_data = linear_lon.compressed_bytes();
        auto linear_lat_data = linear_lat.compressed_bytes();
        SerfQtLinearDecompressor linear_lon_dec;
        SerfQtLinearDecompressor linear_lat_dec;
        auto linear_lon_values = linear_lon_dec.Decompress(linear_lon_data);
        auto linear_lat_values = linear_lat_dec.Decompress(linear_lat_data);
        for (size_t i = 0; i < linear_lon_values.size() && i < linear_lat_values.size(); ++i) {
            linear_decompressed.emplace_back(linear_lon_values[i], linear_lat_values[i]);
        }
        auto linear_decompress_end = std::chrono::steady_clock::now();
        double linear_decompress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            linear_decompress_end - linear_decompress_start).count() / static_cast<double>(gps_data.size());
        double linear_max_error, linear_avg_error;
        int linear_exceeding;
        CalculateErrors(gps_data, linear_decompressed, linear_max_error, linear_avg_error, linear_exceeding, epsilon);
        
        // Serf-QT-Curve
        auto curve_compress_start = std::chrono::steady_clock::now();
        SerfQtCurveCompressor curve_lon(gps_data.size(), epsilon);
        SerfQtCurveCompressor curve_lat(gps_data.size(), epsilon);
        for (const auto& point : gps_data) {
            curve_lon.AddValue(point.longitude, point.timestamp);
            curve_lat.AddValue(point.latitude, point.timestamp);
        }
        curve_lon.Close();
        curve_lat.Close();
        auto curve_compress_end = std::chrono::steady_clock::now();
        double curve_compress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            curve_compress_end - curve_compress_start).count() / static_cast<double>(gps_data.size());
        
 // Serf-QT-Curve 
        auto curve_decompress_start = std::chrono::steady_clock::now();
        std::vector<GpsPoint> curve_decompressed;
        auto curve_lon_data = curve_lon.compressed_bytes();
        auto curve_lat_data = curve_lat.compressed_bytes();
        SerfQtCurveDecompressor curve_lon_dec;
        SerfQtCurveDecompressor curve_lat_dec;
        auto curve_lon_values = curve_lon_dec.Decompress(curve_lon_data);
        auto curve_lat_values = curve_lat_dec.Decompress(curve_lat_data);
        for (size_t i = 0; i < curve_lon_values.size() && i < curve_lat_values.size(); ++i) {
            curve_decompressed.emplace_back(curve_lon_values[i], curve_lat_values[i]);
        }
        auto curve_decompress_end = std::chrono::steady_clock::now();
        double curve_decompress_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            curve_decompress_end - curve_decompress_start).count() / static_cast<double>(gps_data.size());
        double curve_max_error, curve_avg_error;
        int curve_exceeding;
        CalculateErrors(gps_data, curve_decompressed, curve_max_error, curve_avg_error, curve_exceeding, epsilon);
        std::cout << "  [7/8] Linear  (max_err=" << std::scientific << linear_max_error << ")" << std::endl;
        std::cout << "  [8/8] Curve  (max_err=" << std::scientific << curve_max_error << ")" << std::endl;
        
        // (comment removed)
        SummaryResult result;
        result.dataset_name = dataset.name;
        result.points = gps_data.size();
        
 // TrajSP: timestamp bits
        auto sp_stats_final = sp_compressor.GetStats();
        result.sp_bits = sp_stats_final.total_bits - sp_stats_final.timestamp_bits;
        
        result.adaptive_bits = adaptive_bits;  // 
        
 // Simple（）: timestamp bits
        auto simple_stats_final = cost_compressor.GetStats();
        result.simple_bits = simple_stats_final.total_bits - simple_stats_final.timestamp_bits;
        
 // Simple（）: timestamp bits
        auto simple_time_stats_final = simple_time_compressor.GetStats();
        result.simple_time_bits = simple_time_stats_final.total_bits - simple_time_stats_final.timestamp_bits;
        
        result.qt_bits = qt_lon.get_compressed_size_in_bits() + qt_lat.get_compressed_size_in_bits();
        
 // Linear/Curve: timestamp bits (64timestamp，+ = 128/)
        int timestamp_bits_per_point = 128;  // 64 × 2 (+)
        int total_timestamp_bits = timestamp_bits_per_point * gps_data.size();
        result.linear_bits = linear_lon.get_compressed_size_in_bits() + linear_lat.get_compressed_size_in_bits() - total_timestamp_bits;
        result.curve_bits = curve_lon.get_compressed_size_in_bits() + curve_lat.get_compressed_size_in_bits() - total_timestamp_bits;
        result.sp_avg = static_cast<double>(result.sp_bits) / result.points;
        result.adaptive_avg = static_cast<double>(result.adaptive_bits) / result.points;
        result.simple_avg = static_cast<double>(result.simple_bits) / result.points;
        result.simple_time_avg = static_cast<double>(result.simple_time_bits) / result.points;  // 
        result.qt_avg = static_cast<double>(result.qt_bits) / result.points;
        result.linear_avg = static_cast<double>(result.linear_bits) / result.points;
        result.curve_avg = static_cast<double>(result.curve_bits) / result.points;
 // （bits / bits， = 2 doubles = 128 bits/）
        constexpr double kOriginalBitsPerPoint = 128.0;  // 2 doubles (longitude + latitude)
        result.sp_cr = result.sp_avg / kOriginalBitsPerPoint;
        result.adaptive_cr = result.adaptive_avg / kOriginalBitsPerPoint;
        result.cost_cr = result.simple_avg / kOriginalBitsPerPoint;
        result.cost_time_cr = result.simple_time_avg / kOriginalBitsPerPoint;
        result.qt_cr = result.qt_avg / kOriginalBitsPerPoint;
        result.linear_cr = result.linear_avg / kOriginalBitsPerPoint;
        result.curve_cr = result.curve_avg / kOriginalBitsPerPoint;
        // (comment removed)
        result.sp_max_error = sp_max_error;
        result.sp_avg_error = sp_avg_error;
        result.adaptive_max_error = adaptive_max_error;
        result.adaptive_avg_error = adaptive_avg_error;
        result.cost_max_error = cost_max_error;
        result.cost_avg_error = cost_avg_error;
        result.cost_time_max_error = cost_time_max_error;  // 
        result.simple_time_avg_error = simple_time_avg_error;
        result.qt_max_error = qt_max_error;
        result.qt_avg_error = qt_avg_error;
        result.linear_max_error = linear_max_error;
        result.linear_avg_error = linear_avg_error;
        result.curve_max_error = curve_max_error;
        result.curve_avg_error = curve_avg_error;
        // (comment removed)
        result.sp_compress_time = sp_compress_time_us;
        result.sp_decompress_time = sp_decompress_time_us;
        result.adaptive_compress_time = 0.0;  // Adaptive
        result.adaptive_decompress_time = 0.0;
        result.cost_compress_time = cost_compress_time_us;
        result.cost_decompress_time = cost_decompress_time_us;
        result.cost_time_compress_time = cost_time_compress_time_us;
        result.cost_time_decompress_time = cost_time_decompress_time_us;
        result.qt_compress_time = qt_compress_time_us;
        result.qt_decompress_time = qt_decompress_time_us;
        result.linear_compress_time = linear_compress_time_us;
        result.linear_decompress_time = linear_decompress_time_us;
        result.curve_compress_time = curve_compress_time_us;
        result.curve_decompress_time = curve_decompress_time_us;
        
        summary_results.push_back(result);
        
        std::cout << "✓ " << dataset.name << ": " << gps_data.size() << " " << std::endl;
    }
    
    // (comment removed)
    std::cout << "\n" << std::string(100, '=') << std::endl;
    std::cout << " - " << std::endl;
    std::cout << std::string(100, '=') << std::endl;
    std::cout << std::endl;
    
 // CSV
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream csv_filename;
    csv_filename << "compression_results_" 
                 << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S") 
                 << ".csv";
    
 // CSV
    std::ofstream csv_file(csv_filename.str());
    if (csv_file.is_open()) {
 // CSV（BPP + + + ，Adaptive，）
        csv_file << "Dataset,Points,"
                 << "TrajSP_BPP,Simple_BPP,SimpleTime60s_BPP,QT_BPP,Linear_BPP,Curve_BPP,"
                 << "TrajSP_CR,Simple_CR,SimpleTime60s_CR,QT_CR,Linear_CR,Curve_CR,"
                 << "TrajSP_CompressTime(μs/pt),TrajSP_DecompressTime(μs/pt),"
                 << "Simple_CompressTime(μs/pt),Simple_DecompressTime(μs/pt),"
                 << "SimpleTime60s_CompressTime(μs/pt),SimpleTime60s_DecompressTime(μs/pt),"
                 << "QT_CompressTime(μs/pt),QT_DecompressTime(μs/pt),"
                 << "Linear_CompressTime(μs/pt),Linear_DecompressTime(μs/pt),"
                 << "Curve_CompressTime(μs/pt),Curve_DecompressTime(μs/pt),"
                 << "TrajSP_MaxErr,TrajSP_AvgErr,"
                 << "Simple_MaxErr,Simple_AvgErr,"
                 << "SimpleTime60s_MaxErr,SimpleTime60s_AvgErr,"
                 << "QT_MaxErr,QT_AvgErr,"
                 << "Linear_MaxErr,Linear_AvgErr,"
                 << "Curve_MaxErr,Curve_AvgErr,"
                 << "Best\n";
    }
    
    // (comment removed)
    std::cout << std::setw(25) << "" 
              << std::setw(10) << ""
              << std::setw(10) << "TrajSP"
              << std::setw(10) << "Simple96"
              << std::setw(10) << "Time60s"
              << std::setw(10) << "QT()"
              << std::setw(10) << "Linear"
              << std::setw(10) << "Curve"
              << std::setw(12) << "" << std::endl;
    std::cout << std::string(115, '-') << std::endl;
    
    for (const auto& result : summary_results) {
        double min_avg = std::min({result.sp_avg, result.simple_avg, result.simple_time_avg, result.qt_avg, result.linear_avg, result.curve_avg});
        std::string best;
        if (result.sp_avg == min_avg) best = "TrajSP";
        else if (result.simple_avg == min_avg) best = "Simple96";
        else if (result.simple_time_avg == min_avg) best = "Time60s";
        else if (result.qt_avg == min_avg) best = "QT";
        else if (result.linear_avg == min_avg) best = "Linear";
        else best = "Curve";
        
 // （6）
        std::cout << std::setw(25) << result.dataset_name
                  << std::setw(10) << result.points
                  << std::setw(10) << std::fixed << std::setprecision(2) << result.sp_avg
                  << std::setw(10) << result.simple_avg
                  << std::setw(10) << result.simple_time_avg
                  << std::setw(10) << result.qt_avg
                  << std::setw(10) << result.linear_avg
                  << std::setw(10) << result.curve_avg
                  << std::setw(12) << best << std::endl;
        
 // CSV（6，Adaptive，）
        if (csv_file.is_open()) {
            csv_file << result.dataset_name << ","
                     << result.points << ","
                     << std::fixed << std::setprecision(6)
                     << result.sp_avg << ","
                     << result.simple_avg << ","
                     << result.simple_time_avg << ","
                     << result.qt_avg << ","
                     << result.linear_avg << ","
                     << result.curve_avg << ","
                     << result.sp_cr << ","
                     << result.cost_cr << ","
                     << result.cost_time_cr << ","
                     << result.qt_cr << ","
                     << result.linear_cr << ","
                     << result.curve_cr << ","
                     << result.sp_compress_time << ","
                     << result.sp_decompress_time << ","
                     << result.cost_compress_time << ","
                     << result.cost_decompress_time << ","
                     << result.cost_time_compress_time << ","
                     << result.cost_time_decompress_time << ","
                     << result.qt_compress_time << ","
                     << result.qt_decompress_time << ","
                     << result.linear_compress_time << ","
                     << result.linear_decompress_time << ","
                     << result.curve_compress_time << ","
                     << result.curve_decompress_time << ","
                     << std::scientific << std::setprecision(6)
                     << result.sp_max_error << ","
                     << result.sp_avg_error << ","
                     << result.cost_max_error << ","
                     << result.cost_avg_error << ","
                     << result.cost_time_max_error << ","
                     << result.simple_time_avg_error << ","
                     << result.qt_max_error << ","
                     << result.qt_avg_error << ","
                     << result.linear_max_error << ","
                     << result.linear_avg_error << ","
                     << result.curve_max_error << ","
                     << result.curve_avg_error << ","
                     << best << "\n";
        }
    }
    
    std::cout << std::string(100, '-') << std::endl;
    std::cout << ":  (bits/，6)" << std::endl;
    
 // CSV
    if (csv_file.is_open()) {
        csv_file.close();
        std::cout << "\n✅ : " << csv_filename.str() << std::endl;
    }
    
    // (comment removed)
    int sp_wins = 0, adaptive_wins = 0, cost_wins = 0, qt_wins = 0, linear_wins = 0, curve_wins = 0;
    for (const auto& result : summary_results) {
        double min_avg = std::min({result.sp_avg, result.adaptive_avg, result.simple_avg, result.qt_avg, result.linear_avg, result.curve_avg});
        if (result.sp_avg == min_avg) sp_wins++;
        else if (result.adaptive_avg == min_avg) adaptive_wins++;
        else if (result.simple_avg == min_avg) cost_wins++;
        else if (result.qt_avg == min_avg) qt_wins++;
        else if (result.linear_avg == min_avg) linear_wins++;
        else curve_wins++;
    }
    
    std::cout << "\n:" << std::endl;
    std::cout << "  TrajCompress-SP:               " << sp_wins << " " << std::endl;
    std::cout << "  TrajCompress-SP-Adaptive:      " << adaptive_wins << " " << std::endl;
    std::cout << "  TrajCompress-SP-CoST (point-based): " << cost_wins << " " << std::endl;
    std::cout << "  Serf-QT ():                " << qt_wins << " " << std::endl;
    std::cout << "  Serf-QT-Linear:                " << linear_wins << " " << std::endl;
    std::cout << "  Serf-QT-Curve:                 " << curve_wins << " " << std::endl;
    
 // （）
    std::cout << "\n" << std::string(100, '=') << std::endl;
    std::cout << "（ CR = compressed_bits / original_bits）" << std::endl;
    std::cout << std::string(100, '=') << std::endl;
    
    // (comment removed)
    const SummaryResult* geolife_result = nullptr;
    const SummaryResult* trajectory_result = nullptr;
    const SummaryResult* wxtaxi_result = nullptr;
    
    for (const auto& result : summary_results) {
        if (result.dataset_name == "Geolife ()") {
            geolife_result = &result;
        } else if (result.dataset_name == "Trajectory ()") {
            trajectory_result = &result;
        } else if (result.dataset_name == "WX_taxi ()") {
            wxtaxi_result = &result;
        }
    }
    
    if (geolife_result && trajectory_result && wxtaxi_result) {
        std::cout << "\n" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << "Geolife"
                  << std::setw(15) << "Trajectory"
                  << std::setw(15) << "WX-Taxi" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        // (comment removed)
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << std::fixed << std::setprecision(6) << geolife_result->qt_cr
                  << std::setw(15) << trajectory_result->qt_cr
                  << std::setw(15) << wxtaxi_result->qt_cr << std::endl;
        
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << geolife_result->linear_cr
                  << std::setw(15) << trajectory_result->linear_cr
                  << std::setw(15) << wxtaxi_result->linear_cr << std::endl;
        
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << geolife_result->curve_cr
                  << std::setw(15) << trajectory_result->curve_cr
                  << std::setw(15) << wxtaxi_result->curve_cr << std::endl;
        
        // (comment removed)
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << geolife_result->sp_cr
                  << std::setw(15) << trajectory_result->sp_cr
                  << std::setw(15) << wxtaxi_result->sp_cr << std::endl;
        
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << "Cost"
                  << std::setw(15) << geolife_result->cost_cr
                  << std::setw(15) << trajectory_result->cost_cr
                  << std::setw(15) << wxtaxi_result->cost_cr << std::endl;
        
        std::cout << std::string(80, '-') << std::endl;
        std::cout << ": CR = bits / bits (128 bits/)，" << std::endl;
        
        // (comment removed)
        std::cout << "\n\n (μs/)" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << "Geolife"
                  << std::setw(15) << "Trajectory"
                  << std::setw(15) << "WX-Taxi" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        // (comment removed)
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << std::fixed << std::setprecision(6) << geolife_result->qt_compress_time
                  << std::setw(15) << trajectory_result->qt_compress_time
                  << std::setw(15) << wxtaxi_result->qt_compress_time << std::endl;
        
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << geolife_result->linear_compress_time
                  << std::setw(15) << trajectory_result->linear_compress_time
                  << std::setw(15) << wxtaxi_result->linear_compress_time << std::endl;
        
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << geolife_result->curve_compress_time
                  << std::setw(15) << trajectory_result->curve_compress_time
                  << std::setw(15) << wxtaxi_result->curve_compress_time << std::endl;
        
        // (comment removed)
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << geolife_result->sp_compress_time
                  << std::setw(15) << trajectory_result->sp_compress_time
                  << std::setw(15) << wxtaxi_result->sp_compress_time << std::endl;
        
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << "Cost"
                  << std::setw(15) << geolife_result->cost_compress_time
                  << std::setw(15) << trajectory_result->cost_compress_time
                  << std::setw(15) << wxtaxi_result->cost_compress_time << std::endl;
        
        std::cout << std::string(80, '-') << std::endl;
        
        // (comment removed)
        std::cout << "\n\n (μs/)" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << "Geolife"
                  << std::setw(15) << "Trajectory"
                  << std::setw(15) << "WX-Taxi" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        // (comment removed)
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << std::fixed << std::setprecision(6) << geolife_result->qt_decompress_time
                  << std::setw(15) << trajectory_result->qt_decompress_time
                  << std::setw(15) << wxtaxi_result->qt_decompress_time << std::endl;
        
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << geolife_result->linear_decompress_time
                  << std::setw(15) << trajectory_result->linear_decompress_time
                  << std::setw(15) << wxtaxi_result->linear_decompress_time << std::endl;
        
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << geolife_result->curve_decompress_time
                  << std::setw(15) << trajectory_result->curve_decompress_time
                  << std::setw(15) << wxtaxi_result->curve_decompress_time << std::endl;
        
        // (comment removed)
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << ""
                  << std::setw(15) << geolife_result->sp_decompress_time
                  << std::setw(15) << trajectory_result->sp_decompress_time
                  << std::setw(15) << wxtaxi_result->sp_decompress_time << std::endl;
        
        std::cout << std::setw(15) << "" 
                  << std::setw(20) << "Cost"
                  << std::setw(15) << geolife_result->cost_decompress_time
                  << std::setw(15) << trajectory_result->cost_decompress_time
                  << std::setw(15) << wxtaxi_result->cost_decompress_time << std::endl;
        
        std::cout << std::string(80, '-') << std::endl;
        
 // CSV
        std::stringstream paper_table_filename;
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        paper_table_filename << "paper_comparison_table_" 
                             << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S") 
                             << ".csv";
        
        std::ofstream paper_csv(paper_table_filename.str());
        if (paper_csv.is_open()) {
            // Average Compression Ratio
            paper_csv << "\nAverage Compression Ratio\n";
            paper_csv << "Category,Algorithm,Geolife,Trajectory,WX-Taxi\n";
            paper_csv << std::fixed << std::setprecision(6);
            paper_csv << "Single-Predictor,Zero Predictor," 
                      << geolife_result->qt_cr << "," 
                      << trajectory_result->qt_cr << "," 
                      << wxtaxi_result->qt_cr << "\n";
            paper_csv << "Single-Predictor,Linear Predictor," 
                      << geolife_result->linear_cr << "," 
                      << trajectory_result->linear_cr << "," 
                      << wxtaxi_result->linear_cr << "\n";
            paper_csv << "Single-Predictor,Curve Predictor," 
                      << geolife_result->curve_cr << "," 
                      << trajectory_result->curve_cr << "," 
                      << wxtaxi_result->curve_cr << "\n";
            paper_csv << "Multi-Predictor,TrajCompress-SP," 
                      << geolife_result->sp_cr << "," 
                      << trajectory_result->sp_cr << "," 
                      << wxtaxi_result->sp_cr << "\n";
            paper_csv << "Multi-Predictor,CoST," 
                      << geolife_result->cost_cr << "," 
                      << trajectory_result->cost_cr << "," 
                      << wxtaxi_result->cost_cr << "\n";
            
            // Average Compression Time
            paper_csv << "\nAverage Compression Time (μs/point)\n";
            paper_csv << "Category,Algorithm,Geolife,Trajectory,WX-Taxi\n";
            paper_csv << "Single-Predictor,Zero Predictor," 
                      << geolife_result->qt_compress_time << "," 
                      << trajectory_result->qt_compress_time << "," 
                      << wxtaxi_result->qt_compress_time << "\n";
            paper_csv << "Single-Predictor,Linear Predictor," 
                      << geolife_result->linear_compress_time << "," 
                      << trajectory_result->linear_compress_time << "," 
                      << wxtaxi_result->linear_compress_time << "\n";
            paper_csv << "Single-Predictor,Curve Predictor," 
                      << geolife_result->curve_compress_time << "," 
                      << trajectory_result->curve_compress_time << "," 
                      << wxtaxi_result->curve_compress_time << "\n";
            paper_csv << "Multi-Predictor,TrajCompress-SP," 
                      << geolife_result->sp_compress_time << "," 
                      << trajectory_result->sp_compress_time << "," 
                      << wxtaxi_result->sp_compress_time << "\n";
            paper_csv << "Multi-Predictor,CoST," 
                      << geolife_result->cost_compress_time << "," 
                      << trajectory_result->cost_compress_time << "," 
                      << wxtaxi_result->cost_compress_time << "\n";
            
            // Average Decompression Time
            paper_csv << "\nAverage Decompression Time (μs/point)\n";
            paper_csv << "Category,Algorithm,Geolife,Trajectory,WX-Taxi\n";
            paper_csv << "Single-Predictor,Zero Predictor," 
                      << geolife_result->qt_decompress_time << "," 
                      << trajectory_result->qt_decompress_time << "," 
                      << wxtaxi_result->qt_decompress_time << "\n";
            paper_csv << "Single-Predictor,Linear Predictor," 
                      << geolife_result->linear_decompress_time << "," 
                      << trajectory_result->linear_decompress_time << "," 
                      << wxtaxi_result->linear_decompress_time << "\n";
            paper_csv << "Single-Predictor,Curve Predictor," 
                      << geolife_result->curve_decompress_time << "," 
                      << trajectory_result->curve_decompress_time << "," 
                      << wxtaxi_result->curve_decompress_time << "\n";
            paper_csv << "Multi-Predictor,TrajCompress-SP," 
                      << geolife_result->sp_decompress_time << "," 
                      << trajectory_result->sp_decompress_time << "," 
                      << wxtaxi_result->sp_decompress_time << "\n";
            paper_csv << "Multi-Predictor,CoST," 
                      << geolife_result->cost_decompress_time << "," 
                      << trajectory_result->cost_decompress_time << "," 
                      << wxtaxi_result->cost_decompress_time << "\n";
            
            paper_csv.close();
            std::cout << "\n✅ : " << paper_table_filename.str() << std::endl;
        }
    } else {
        std::cout << "\n⚠️  : " << std::endl;
    }
}

int main(int argc, char* argv[]) {
    double epsilon = 1e-5;   // 1e-5 1.1，GPS
    
    std::cout << "TrajCompress-SP " << std::endl;
    std::cout << std::string(100, '=') << std::endl;
    
    // (comment removed)
    if (argc > 1) {
        std::string mode = argv[1];
        
        if (mode == "all") {
            // (comment removed)
            if (argc > 2) {
                epsilon = std::stod(argv[2]);
            }
            TestAllDatasetsAndGenerateSummary(epsilon);
        } else if (mode == "single") {
            // Test single dataset (with timestamp)
            std::string dataset_path = "../../data/Geolife_100k_with_id.csv";
            int max_points = -1;
            
    if (argc > 2) {
                dataset_path = argv[2];
    }
    if (argc > 3) {
                max_points = std::stoi(argv[3]);
            }
            if (argc > 4) {
                epsilon = std::stod(argv[4]);
            }
            
    auto gps_data = LoadGpsDataFromCSV(dataset_path, max_points);
    if (gps_data.empty()) {
        std::cerr << "GPS，" << std::endl;
        return 1;
    }
    
            // (comment removed)
            TestTrajCompressSP(ToGpsPointVector(gps_data), epsilon);
 // TestTrajCompressSPAdaptive(gps_data, epsilon); //
    TestSerfQT(ToGpsPointVector(gps_data), epsilon);
            TestSerfQTLinear(ToGpsPointVector(gps_data), epsilon);
            TestSerfQTCurve(ToGpsPointVector(gps_data), epsilon);
            
            // (comment removed)
            FiveWayComparativeTest(ToGpsPointVector(gps_data), epsilon);
            
 // （）
            FourWayComparativeTest(ToGpsPointVector(gps_data), epsilon);
    ComparativeTest(ToGpsPointVector(gps_data), epsilon);
        } else {
            std::cout << ":" << std::endl;
            std::cout << "  : " << argv[0] << " all [epsilon]" << std::endl;
            std::cout << "  : " << argv[0] << " single <> [] [epsilon]" << std::endl;
            std::cout << "\n:" << std::endl;
            std::cout << "  " << argv[0] << " all 1e-5" << std::endl;
            std::cout << "  " << argv[0] << " single test/data_set/Geolife_100k_longitude_latitude.csv 10000 1e-5" << std::endl;
            return 1;
        }
    } else {
 // ：（）
        std::cout << ": " << std::endl;
        std::cout << ":  './trajcompress_sp_test all'  './trajcompress_sp_test single <>' " << std::endl;
        TestAllDatasetsAndGenerateSummary(epsilon);
    }
    
    std::cout << "\n！" << std::endl;
    
    return 0;
}

