/**
 * Comprehensive Performance Test V3
 * 
 * This file implements comprehensive performance comparison experiments for CoST
 * against multiple baseline algorithms.
 * 
 * Features:
 * 1. Tests multiple baseline algorithms (Chimp128, Elf, Serf, SZ2, Machete, etc.)
 * 2. Evaluates compression ratio, compression/decompression time, and error metrics
 * 3. Supports both streaming and batch processing algorithms
 * 4. Configuration: comprehensive_perf_config.hpp
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <iomanip>
#include <cmath>

#include "comprehensive_perf_config.hpp"

// Serf （Block-based）
#include "../../baselines/serf/serf_qt_compressor.h"
#include "../../baselines/serf/serf_qt_decompressor.h"
#include "../../baselines/serf/serf_xor_compressor.h"
#include "../../baselines/serf/serf_xor_decompressor.h"

// Serf （， block-size）
#include "../../baselines/serf/net_serf_qt_compressor.h"
#include "../../baselines/serf/net_serf_qt_decompressor.h"
#include "../../baselines/serf/net_serf_xor_compressor.h"
#include "../../baselines/serf/net_serf_xor_decompressor.h"

// CoST （decompressor）
#include "../../baselines/serf/../../algorithm/cost_compressor.h"

// 
#include "../../baselines/gorilla/gorilla_compressor.h"
#include "../../baselines/gorilla/gorilla_decompressor.h"
#include "../../baselines/chimp128/chimp_compressor.h"
#include "../../baselines/chimp128/chimp_decompressor.h"
#include "../../baselines/deflate/deflate_compressor.h"
#include "../../baselines/deflate/deflate_decompressor.h"
#include "../../baselines/lz4/lz4_compressor.h"
#include "../../baselines/lz4/lz4_decompressor.h"
#include "../../baselines/fpc/fpc_compressor.h"
#include "../../baselines/fpc/fpc_decompressor.h"
#include "../../baselines/lz77/fastlz.h"
#include "../../baselines/zstd/lib/zstd.h"
#include "../../baselines/snappy/snappy.h"
#include "../../baselines/sz2/sz/include/sz.h"
#include "../../baselines/machete/machete.h"
#include "../../baselines/sim_piece/sim_piece.h"
#include "../../baselines/sprintz/double_sprintz_compressor.h"
#include "../../baselines/sprintz/double_sprintz_decompressor.h"
#include "../../baselines/elf/elf.h"

// Unix（）
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

// （ Perf.cc ）
struct PerfRecord {
    long total_compressed_bits = 0;
    long total_compression_time_us = 0;
    long total_decompression_time_us = 0;
    int block_count = 0;
    int total_points = 0;  // ：（Simple）
    
    void AddCompressedSize(long bits) { total_compressed_bits += bits; }
    void AddCompressionTime(long us) { total_compression_time_us += us; }
    void AddDecompressionTime(long us) { total_decompression_time_us += us; }
    
    double AvgCompressionTimePerBlock() const {
        return block_count > 0 ? (total_compression_time_us / 1000.0) / block_count : 0;
    }
    
    double AvgDecompressionTimePerBlock() const {
        return block_count > 0 ? (total_decompression_time_us / 1000.0) / block_count : 0;
    }
    
    // （）
    double AvgCompressionTimePerPoint() const {
        return total_points > 0 ? static_cast<double>(total_compression_time_us) / total_points : 0;
    }
    
    // （）
    double AvgDecompressionTimePerPoint() const {
        return total_points > 0 ? static_cast<double>(total_decompression_time_us) / total_points : 0;
    }
    
    double CompressionRatio(int block_size) const {
        long original_bits = static_cast<long>(block_count) * block_size * 64;
        //  = bits / bits（）
        return original_bits > 0 ? static_cast<double>(total_compressed_bits) / original_bits : 0;
    }
    
    // ：（Simple）
    // ： = bits / bits（）
    double CompressionRatioByPoints() const {
        // Simple：total_points  GPS（）
        // bits = GPS * 128 (2double)
        long original_total_bits = static_cast<long>(total_points) * 128;
        //  = bits / bits
        return original_total_bits > 0 ? static_cast<double>(total_compressed_bits) / original_total_bits : 0;
    }
    
    // BPP (Bits Per Point) - GPSbits
    double BitsPerPoint() const {
        // ：Simple，total_pointsGPS，2
        return total_points > 0 ? static_cast<double>(total_compressed_bits) / total_points : 0;
    }
    
    double BitsPerValue(int block_size) const {
        int total_values = block_count * block_size;
        return total_values > 0 ? static_cast<double>(total_compressed_bits) / total_values : 0;
    }
};

// 2D（）
struct PerfRecord2D {
    PerfRecord longitude_record;
    PerfRecord latitude_record;
    
    // ： = bits / bits（）
    // 1D，bits = bits + bits
    double AvgCompressionRatio(int block_size) const {
        //  total_points  block_count * block_size（）
        int gps_points = longitude_record.total_points;  // 1D 
        long original_total_bits = static_cast<long>(gps_points) * 128;  // 2double
        long compressed_total_bits = longitude_record.total_compressed_bits + 
                                     latitude_record.total_compressed_bits;
        return original_total_bits > 0 ? static_cast<double>(compressed_total_bits) / original_total_bits : 0;
    }
    
    double AvgCompressionTime() const {
        return (longitude_record.AvgCompressionTimePerBlock() + 
                latitude_record.AvgCompressionTimePerBlock()) / 2.0;
    }
    
    double AvgDecompressionTime() const {
        return (longitude_record.AvgDecompressionTimePerBlock() + 
                latitude_record.AvgDecompressionTimePerBlock()) / 2.0;
    }
    
    // GPS（）- 
    double AvgCompressionTimePerPoint() const {
        return (longitude_record.AvgCompressionTimePerPoint() + 
                latitude_record.AvgCompressionTimePerPoint()) / 2.0;
    }
    
    // GPS（）- 
    double AvgDecompressionTimePerPoint() const {
        return (longitude_record.AvgDecompressionTimePerPoint() + 
                latitude_record.AvgDecompressionTimePerPoint()) / 2.0;
    }
};

// CSVblock（ ReadBlock）
std::vector<double> ReadBlock(std::ifstream &file, int block_size, int column) {
    std::vector<double> ret;
    ret.reserve(block_size);
    int entry_count = 0;
    std::string line;
    
    while (!file.eof() && entry_count < block_size) {
        if (!std::getline(file, line)) break;
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string value_str;
        int col_idx = 0;
        
        while (std::getline(ss, value_str, ',')) {
            if (col_idx == column) {
                try {
                    double value = std::stod(value_str);
                    ret.emplace_back(value);
                    ++entry_count;
                } catch (...) {
                    // 
                }
                break;
            }
            col_idx++;
        }
    }
    
    return ret;
}

//  block_size（）
// dataset_size: 
// default_block_size:  block_size（ min(dataset_size, kStreamingBlockSizeMax)）
int GetAlgorithmBlockSize(const std::string& algo_name, int dataset_size, int default_block_size) {
    auto it = kAlgorithmSpecificBlockSizeMax.find(algo_name);
    if (it != kAlgorithmSpecificBlockSizeMax.end()) {
        int limit = it->second;
        if (limit == -1) {
            // -1 （）
            return dataset_size;
        } else {
            // 
            return std::min(default_block_size, limit);
        }
    }
    // ，
    return default_block_size;
}

// （-1，header）
int GetDatasetSize(const std::string& filepath) {
    std::cout << "    : " << filepath << "\n";
    std::cout.flush();
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "    ❌ \n";
        return 0;
    }
    
    int count = 0;
    std::string line;
    std::getline(file, line);  // header
    
    while (std::getline(file, line)) {
        if (!line.empty()) ++count;
        if (count % 10000 == 0) {
            std::cout << "     " << count << " ...\r";
            std::cout.flush();
        }
    }
    
    file.close();
    std::cout << "    ✅ : " << count << " \n";
    std::cout.flush();
    return count;
}

// （ ResetFileStream）
void ResetFileStream(std::ifstream &file) {
    file.clear();
    file.seekg(0, std::ios::beg);
    // header
    std::string line;
    std::getline(file, line);
}

// 2D GPS（++）
std::vector<CoSTCompressor::GpsPoint> ReadBlock2D(
    std::ifstream &file, int block_size) {
    std::vector<CoSTCompressor::GpsPoint> ret;
    ret.reserve(block_size);
    int entry_count = 0;
    std::string line;
    
    while (!file.eof() && entry_count < block_size) {
        if (!std::getline(file, line)) break;
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string lon_str, lat_str, ts_str;
        
        if (std::getline(ss, lon_str, ',') && 
            std::getline(ss, lat_str, ',') && 
            std::getline(ss, ts_str, ',')) {
            try {
                double longitude = std::stod(lon_str);
                double latitude = std::stod(lat_str);
                uint64_t timestamp = std::stoull(ts_str);
                ret.emplace_back(longitude, latitude, timestamp);
                ++entry_count;
            } catch (...) {
                // 
            }
        }
    }
    
    return ret;
}

//  Serf-XOR（block ， Perf.cc）
void PerfSerfXOR(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                 const std::string &data_set, int column, PerfRecord &perf_record) {
    // ：SerfXOR  windows_size (1000)， block_size！
    SerfXORCompressor serf_xor_compressor(1000, max_diff, 0);  // adjust_digit = 0
    SerfXORDecompressor serf_xor_decompressor(0);
    
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        
        auto compression_start_time = std::chrono::steady_clock::now();
        for (const auto &value : original_data) serf_xor_compressor.AddValue(value);
        serf_xor_compressor.Close();
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(serf_xor_compressor.compressed_size_last_block());
        Array<uint8_t> compression_output = serf_xor_compressor.compressed_bytes_last_block();
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        std::vector<double> decompressed_data = serf_xor_decompressor.Decompress(compression_output);
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  Serf-QT（ block-based ）
void PerfSerfQt(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                const std::string &data_set, int column, PerfRecord &perf_record) {
    if (kUseFullDatasetCompression) {
        // 
        std::vector<double> all_data;
        std::string line;
        
        // 
        while (std::getline(data_set_input_stream_ref, line)) {
            if (line.empty()) continue;
            
            std::stringstream ss(line);
            std::string value_str;
            
            // 
            for (int i = 0; i <= column; ++i) {
                if (!std::getline(ss, value_str, ',')) {
                    break;
                }
            }
            
            try {
                double value = std::stod(value_str);
                all_data.push_back(value);
            } catch (...) {
                continue;
            }
        }
        
        if (all_data.empty()) {
            ResetFileStream(data_set_input_stream_ref);
            return;
        }
        
        // 
        SerfQtCompressor serf_qt_compressor(all_data.size(), max_diff);
        SerfQtDecompressor serf_qt_decompressor;
        
        auto compression_start_time = std::chrono::steady_clock::now();
        for (const auto &value : all_data) {
            serf_qt_compressor.AddValue(value);
        }
        serf_qt_compressor.Close();
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(serf_qt_compressor.get_compressed_size_in_bits());
        Array<uint8_t> compression_output = serf_qt_compressor.compressed_bytes();
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        std::vector<double> decompressed_data = serf_qt_decompressor.Decompress(compression_output);
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time.count());
        perf_record.AddDecompressionTime(decompression_time.count());
        perf_record.total_points = all_data.size();
        perf_record.block_count = 1;
        
    } else {
        // Block-based （ Perf.cc）
        SerfQtCompressor serf_qt_compressor(block_size, max_diff);
        SerfQtDecompressor serf_qt_decompressor;
        
        int block_count = 0;
        std::vector<double> original_data;
        
        while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
            ++block_count;
            
            auto compression_start_time = std::chrono::steady_clock::now();
            for (const auto &value : original_data) serf_qt_compressor.AddValue(value);
            serf_qt_compressor.Close();
            auto compression_end_time = std::chrono::steady_clock::now();
            
            perf_record.AddCompressedSize(serf_qt_compressor.get_compressed_size_in_bits());
            Array<uint8_t> compression_output = serf_qt_compressor.compressed_bytes();
            
            auto decompression_start_time = std::chrono::steady_clock::now();
            std::vector<double> decompressed_data = serf_qt_decompressor.Decompress(compression_output);
            auto decompression_end_time = std::chrono::steady_clock::now();
            
            auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
                compression_end_time - compression_start_time);
            auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
                decompression_end_time - decompression_start_time);
            
            perf_record.AddCompressionTime(compression_time_in_a_block.count());
            perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
            perf_record.total_points += block_size;
        }
        
        perf_record.block_count = block_count;
    }
    
    ResetFileStream(data_set_input_stream_ref);
}

//  Gorilla（ PerfGorilla）
void PerfGorilla(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                 const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        GorillaCompressor gorilla_compressor(block_size);
        GorillaDecompressor gorilla_decompressor;
        ++block_count;
        
        auto compression_start_time = std::chrono::steady_clock::now();
        for (const auto &value : original_data) {
            gorilla_compressor.addValue(value);
        }
        gorilla_compressor.close();
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(gorilla_compressor.get_compress_size_in_bits());
        Array<uint8_t> compression_output = gorilla_compressor.get_compress_pack();
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        std::vector<double> decompression_output = gorilla_decompressor.decompress(compression_output);
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  Chimp
void PerfChimp128(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                  const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        ChimpCompressor chimp_compressor(128);
        
        auto compression_start_time = std::chrono::steady_clock::now();
        for (const auto &value : original_data) {
            chimp_compressor.addValue(value);
        }
        chimp_compressor.close();
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(chimp_compressor.get_size());
        Array<uint8_t> compression_output = chimp_compressor.get_compress_pack();
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        ChimpDecompressor chimp_decompressor(compression_output, 128);
        std::vector<double> decompression_output = chimp_decompressor.decompress();
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  Deflate
void PerfDeflate(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                 const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        
        DeflateCompressor deflate_compressor(block_size);
        DeflateDecompressor deflate_decompressor;
        
        auto compression_start_time = std::chrono::steady_clock::now();
        for (const auto &value : original_data) deflate_compressor.addValue(value);
        deflate_compressor.close();
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(deflate_compressor.getCompressedSizeInBits());
        Array<uint8_t> compression_output = deflate_compressor.getBytes();
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        std::vector<double> decompressed_data = deflate_decompressor.decompress(compression_output);
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  LZ4
void PerfLZ4(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
             const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        
        LZ4Compressor lz_4_compressor(block_size);
        LZ4Decompressor lz_4_decompressor;
        
        auto compression_start_time = std::chrono::steady_clock::now();
        for (const auto &value : original_data) lz_4_compressor.addValue(value);
        lz_4_compressor.close();
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(lz_4_compressor.getCompressedSizeInBits());
        Array<char> compression_output = lz_4_compressor.getBytes();
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        std::vector<double> decompressed_data = lz_4_decompressor.decompress(compression_output);
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  FPC
void PerfFPC(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
             const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        FpcCompressor fpc_compressor(5, block_size);
        FpcDecompressor fpc_decompressor(5, block_size);
        
        auto compression_start_time = std::chrono::steady_clock::now();
        for (const auto &value : original_data) fpc_compressor.addValue(value);
        fpc_compressor.close();
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(fpc_compressor.getCompressedSizeInBits());
        std::vector<char> compression_output = fpc_compressor.getBytes();
        fpc_decompressor.setBytes(compression_output.data(), compression_output.size());
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        std::vector<double> decompressed_data = fpc_decompressor.decompress();
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  LZ77
void PerfLZ77(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
              const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        auto *compression_output = new uint8_t[10000];
        auto *decompression_output = new double[1000];
        
        auto compression_start_time = std::chrono::steady_clock::now();
        int compression_output_len = fastlz_compress_level(2, original_data.data(), block_size * sizeof(double),
                                                           compression_output);
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(compression_output_len * 8);
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        fastlz_decompress(compression_output, compression_output_len, decompression_output,
                          block_size * sizeof(double));
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
        
        delete[] compression_output;
        delete[] decompression_output;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  Zstd
void PerfZstd(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
              const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        char compression_output[block_size * 10];
        double decompression_output[block_size];
        
        auto compression_start_time = std::chrono::steady_clock::now();
        size_t compression_output_len = ZSTD_compress(compression_output, block_size * 10, original_data.data(),
                                                      original_data.size() * sizeof(double), 3);
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(compression_output_len * 8);
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        ZSTD_decompress(decompression_output, block_size * sizeof(double), compression_output, compression_output_len);
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  Snappy
void PerfSnappy(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        std::string compression_output;
        std::string decompression_output;
        
        auto compression_start_time = std::chrono::steady_clock::now();
        size_t compression_output_len = snappy::Compress(reinterpret_cast<const char *>(original_data.data()),
                                                         original_data.size() * sizeof(double), &compression_output);
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(compression_output_len * 8);
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        snappy::Uncompress(compression_output.data(), compression_output.size(), &decompression_output);
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  SZ2
#if 0  // SZ2 disabled
void PerfSZ2(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
             const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        size_t compression_output_len;
        auto decompression_output = new double[block_size];
        
        auto compression_start_time = std::chrono::steady_clock::now();
        auto compression_output = SZ_compress_args(SZ_DOUBLE, original_data.data(), &compression_output_len,
                                                   ABS, max_diff * 0.99, 0, 0, 0, 0, 0, 0, original_data.size());
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(compression_output_len * 8);
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        size_t decompression_output_len = SZ_decompress_args(SZ_DOUBLE, compression_output,
                                                             compression_output_len, decompression_output, 0, 0,
                                                             0, 0, block_size);
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
        
        delete[] decompression_output;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}
#endif

//  Machete
#if 0  // Machete disabled
void PerfMachete(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                 const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        auto *compression_buffer = new uint8_t[100000];
        auto *decompression_buffer = new double[block_size];
        
        auto compression_start_time = std::chrono::steady_clock::now();
        ssize_t compression_output_len = machete_compress<lorenzo1, hybrid>(original_data.data(), original_data.size(),
                                                                            &compression_buffer, max_diff);
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(compression_output_len * 8);
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        ssize_t decompression_output_len = machete_decompress<lorenzo1, hybrid>(compression_buffer,
                                                                                compression_output_len,
                                                                                decompression_buffer);
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
        
        delete[] compression_buffer;
        delete[] decompression_buffer;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}
#endif

//  SimPiece
void PerfSimPiece(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                  const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        
        std::vector<Point> input_points;
        for (int i = 0; i < original_data.size(); ++i) {
            input_points.emplace_back(i, original_data[i]);
        }
        
        char *compression_output = new char[original_data.size() * 8];
        int compression_output_len = 0;
        int timestamp_store_size;
        
        auto compression_start_time = std::chrono::steady_clock::now();
        SimPiece sim_piece_compress(input_points, max_diff);
        compression_output_len = sim_piece_compress.toByteArray(compression_output, true, &timestamp_store_size);
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize((compression_output_len - timestamp_store_size) * 8);
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        SimPiece sim_piece_decompress(compression_output, compression_output_len, true);
        sim_piece_decompress.decompress();
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
        
        delete[] compression_output;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  Sprintz ( x86 intrinsics)
#ifdef USE_X86_INTRINSICS
void PerfSprintz(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                 const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        DoubleSprintzCompressor sprintz_compressor(max_diff);
        auto *compression_output = new int16_t[original_data.size() * 8];
        DoubleSprintzDecompressor sprintz_decompressor;
        
        auto compression_start_time = std::chrono::steady_clock::now();
        int compression_size = sprintz_compressor.compress(original_data, compression_output);
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(compression_size * 8);
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        sprintz_decompressor.decompress(compression_output);
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
        
        delete[] compression_output;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}
#endif

//  Elf
void PerfElf(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
             const std::string &data_set, int column, PerfRecord &perf_record) {
    int block_count = 0;
    std::vector<double> original_data;
    
    while ((original_data = ReadBlock(data_set_input_stream_ref, block_size, column)).size() == block_size) {
        ++block_count;
        
        uint8_t *compression_output_buffer;
        double *decompression_output = new double[block_size];
        ssize_t compression_output_len_in_bytes;
        ssize_t decompression_len;
        
        auto compression_start_time = std::chrono::steady_clock::now();
        compression_output_len_in_bytes = elf_encode(original_data.data(), original_data.size(),
                                                     &compression_output_buffer, 0);
        auto compression_end_time = std::chrono::steady_clock::now();
        
        perf_record.AddCompressedSize(compression_output_len_in_bytes * 8);
        
        auto decompression_start_time = std::chrono::steady_clock::now();
        decompression_len = elf_decode(compression_output_buffer, compression_output_len_in_bytes, decompression_output, 0);
        delete[] decompression_output;
        auto decompression_end_time = std::chrono::steady_clock::now();
        
        auto compression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            compression_end_time - compression_start_time);
        auto decompression_time_in_a_block = std::chrono::duration_cast<std::chrono::microseconds>(
            decompression_end_time - decompression_start_time);
        
        perf_record.AddCompressionTime(compression_time_in_a_block.count());
        perf_record.AddDecompressionTime(decompression_time_in_a_block.count());
        perf_record.total_points += block_size;
    }
    
    perf_record.block_count = block_count;
    ResetFileStream(data_set_input_stream_ref);
}

//  NetSerf-QT（， block-size）
void PerfNetSerfQt(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                   const std::string &data_set, int column, PerfRecord &perf_record) {
    // 
    std::vector<double> all_data;
    std::string line;
    
    while (std::getline(data_set_input_stream_ref, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string value_str;
        
        // 
        for (int i = 0; i <= column; ++i) {
            if (!std::getline(ss, value_str, ',')) {
                break;
            }
        }
        
        try {
            double value = std::stod(value_str);
            all_data.push_back(value);
        } catch (...) {
            continue;
        }
    }
    
    if (all_data.empty()) {
        ResetFileStream(data_set_input_stream_ref);
        return;
    }
    
    // ：
    NetSerfQtCompressor net_compressor(max_diff);
    NetSerfQtDecompressor net_decompressor(max_diff);
    
    std::vector<Array<uint8_t>> compressed_packets;
    long total_bits = 0;
    
    auto compression_start_time = std::chrono::steady_clock::now();
    for (const auto &value : all_data) {
        Array<uint8_t> packet = net_compressor.Compress(value);
        compressed_packets.push_back(packet);
        //  bit ， 4-bit header
        int actual_bits = net_compressor.GetLastCompressedBits();
        total_bits += (actual_bits > 4 ? actual_bits - 4 : actual_bits);
    }
    auto compression_end_time = std::chrono::steady_clock::now();
    
    perf_record.AddCompressedSize(total_bits);
    
    // 
    auto decompression_start_time = std::chrono::steady_clock::now();
    std::vector<double> decompressed_data;
    for (auto &packet : compressed_packets) {  // ： const
        double value = net_decompressor.Decompress(packet);
        decompressed_data.push_back(value);
    }
    auto decompression_end_time = std::chrono::steady_clock::now();
    
    auto compression_time = std::chrono::duration_cast<std::chrono::microseconds>(
        compression_end_time - compression_start_time);
    auto decompression_time = std::chrono::duration_cast<std::chrono::microseconds>(
        decompression_end_time - decompression_start_time);
    
    perf_record.AddCompressionTime(compression_time.count());
    perf_record.AddDecompressionTime(decompression_time.count());
    perf_record.total_points = all_data.size();
    perf_record.block_count = 1;
    
    ResetFileStream(data_set_input_stream_ref);
}

//  NetSerf-XOR（， block-size）
void PerfNetSerfXOR(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                    const std::string &data_set, int column, PerfRecord &perf_record) {
    // 
    std::vector<double> all_data;
    std::string line;
    
    while (std::getline(data_set_input_stream_ref, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string value_str;
        
        // 
        for (int i = 0; i <= column; ++i) {
            if (!std::getline(ss, value_str, ',')) {
                break;
            }
        }
        
        try {
            double value = std::stod(value_str);
            all_data.push_back(value);
        } catch (...) {
            continue;
        }
    }
    
    if (all_data.empty()) {
        ResetFileStream(data_set_input_stream_ref);
        return;
    }
    
    // ：
    NetSerfXORCompressor net_compressor(kSerfXORWindowSize, max_diff, 0);  // adjust_digit = 0
    NetSerfXORDecompressor net_decompressor(kSerfXORWindowSize, 0);  // ：
    
    std::vector<Array<uint8_t>> compressed_packets;
    long total_bits = 0;
    
    auto compression_start_time = std::chrono::steady_clock::now();
    for (const auto &value : all_data) {
        Array<uint8_t> packet = net_compressor.Compress(value);
        compressed_packets.push_back(packet);
        //  bit ， 4-bit header
        int actual_bits = net_compressor.GetLastCompressedBits();
        total_bits += (actual_bits > 4 ? actual_bits - 4 : actual_bits);
    }
    auto compression_end_time = std::chrono::steady_clock::now();
    
    perf_record.AddCompressedSize(total_bits);
    
    // 
    auto decompression_start_time = std::chrono::steady_clock::now();
    std::vector<double> decompressed_data;
    for (auto &packet : compressed_packets) {  // ： const
        double value = net_decompressor.Decompress(packet);
        decompressed_data.push_back(value);
    }
    auto decompression_end_time = std::chrono::steady_clock::now();
    
    auto compression_time = std::chrono::duration_cast<std::chrono::microseconds>(
        compression_end_time - compression_start_time);
    auto decompression_time = std::chrono::duration_cast<std::chrono::microseconds>(
        decompression_end_time - decompression_start_time);
    
    perf_record.AddCompressionTime(compression_time.count());
    perf_record.AddDecompressionTime(decompression_time.count());
    perf_record.total_points = all_data.size();
    perf_record.block_count = 1;
    
    ResetFileStream(data_set_input_stream_ref);
}

//  CoST（2D GPS）
// ：Simple，
// ：（blockHuffman）
void PerfCoST(std::ifstream &data_set_input_stream_ref, double max_diff, int block_size,
                const std::string &data_set, PerfRecord &perf_record) {
    // （）
    std::vector<CoSTCompressor::GpsPoint> original_data;
    std::string line;
    
    // 
    while (std::getline(data_set_input_stream_ref, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string lon_str, lat_str, ts_str;
        
        if (std::getline(ss, lon_str, ',') && 
            std::getline(ss, lat_str, ',') && 
            std::getline(ss, ts_str, ',')) {
            try {
                double longitude = std::stod(lon_str);
                double latitude = std::stod(lat_str);
                uint64_t timestamp = ParseTimestamp(ts_str);
                original_data.emplace_back(longitude, latitude, timestamp);
            } catch (...) {
                continue;
            }
        }
    }
    
    if (original_data.empty()) {
        ResetFileStream(data_set_input_stream_ref);
        return;
    }
    
    // 
    CoSTCompressor cost_compressor(
        original_data.size(),        // block_size = 
        max_diff,                    // epsilon
        kCoSTEvaluationWindow,     // evaluation_window
        kCoSTUseTimeWindow         // use_time_window
    );
    
    auto compression_start_time = std::chrono::steady_clock::now();
    for (const auto &point : original_data) {
        cost_compressor.AddGpsPoint(point);
    }
    cost_compressor.Close();
    auto compression_end_time = std::chrono::steady_clock::now();
    
    // timestamp bits
    auto cost_stats = cost_compressor.GetStats();
    int spatial_bits = cost_stats.total_bits - cost_stats.timestamp_bits;
    perf_record.AddCompressedSize(spatial_bits);
    
    Array<uint8_t> compressed_data = cost_compressor.GetCompressedData();
    
    auto decompression_start_time = std::chrono::steady_clock::now();
    CoSTDecompressor cost_decompressor(&compressed_data[0], compressed_data.length());
    std::vector<CoSTCompressor::GpsPoint> decompressed_data;
    CoSTCompressor::GpsPoint point;
    
    for (size_t i = 0; i < original_data.size(); ++i) {
        if (cost_decompressor.ReadNextPoint(point)) {
            decompressed_data.push_back(point);
        } else {
            break;
        }
    }
    auto decompression_end_time = std::chrono::steady_clock::now();
    
    auto compression_time = std::chrono::duration_cast<std::chrono::microseconds>(
        compression_end_time - compression_start_time);
    auto decompression_time = std::chrono::duration_cast<std::chrono::microseconds>(
        decompression_end_time - decompression_start_time);
    
    perf_record.AddCompressionTime(compression_time.count());
    perf_record.AddDecompressionTime(decompression_time.count());
    perf_record.total_points = original_data.size();
    perf_record.block_count = 1;  // "block"（）
    
    ResetFileStream(data_set_input_stream_ref);
}

// 
void RunComprehensiveTest() {
    std::cout << "====================================================================================================\n";
    std::cout << " V3 ( Perf.cc)\n";
    std::cout << "====================================================================================================\n";
    std::cout << ":\n";
    std::cout << "  -  Block Size: " << kBatchProcessingBlockSize << "\n";
    std::cout << "  -  Block Size: \n";
    std::cout << "  - Max Diff: " << kMaxDiffOverall << "\n";
    std::cout << "  - SerfXOR Window Size: " << kSerfXORWindowSize << "\n";
    std::cout << "====================================================================================================\n";
    
    // CSV
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream csv_filename;
    csv_filename << kExportFilePrefix 
                 << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S") 
                 << kExportFileSuffix;
    
    std::ofstream csv_file(csv_filename.str());
    csv_file << "Method,DataSet,BlockSize,MaxDiff,CompressionRatio,BPP,CompressionTime(μs/point),DecompressionTime(μs/point)\n";
    
    // 
    for (const auto& config : kDataSetList) {
        std::cout << "\n: " << config.name << "\n";
        std::cout << std::string(100, '-') << "\n";
        
        // 
        int dataset_size = GetDatasetSize(config.path);
        std::cout << "  : " << dataset_size << " \n";
        
        //  block_size
        int batch_block_size = kBatchProcessingBlockSize;  // ：50
        int streaming_block_size = kUseDatasetSizeForStreaming 
            ? std::min(dataset_size, kStreamingBlockSizeMax)  // ：min(, 10000)
            : 5000;
        
        std::cout << "   block_size: " << batch_block_size << "\n";
        std::cout << "   block_size: " << streaming_block_size 
                  << " (min(=" << dataset_size << ", =" << kStreamingBlockSizeMax << "))\n";
        
        // 2D
        std::map<std::string, PerfRecord2D> results_2d;
        
        std::cout << "  ...\n";
        
        // lambda，
        auto TestLongitude = [&](const std::string& algo_name, auto test_func, int default_block_size) {
            int actual_block_size = GetAlgorithmBlockSize(algo_name, dataset_size, default_block_size);
            std::cout << "     " << algo_name << " (block_size=" << actual_block_size;
            if (actual_block_size != default_block_size) {
                if (actual_block_size == dataset_size) {
                    std::cout << ", ";
                } else {
                    std::cout << ",  " << default_block_size;
                }
            }
            std::cout << ")...\n"; std::cout.flush();
            auto start_time = std::chrono::steady_clock::now();
            std::ifstream stream(config.path);
            if (!stream.is_open()) {
                std::cerr << "    ❌ \n";
                return;
            }
            std::string header;
            std::getline(stream, header);
            test_func(stream, kMaxDiffOverall, actual_block_size, config.name, 0, results_2d[algo_name].longitude_record);
            stream.close();
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cout << "    ✅ " << algo_name << "  (: " << duration.count() << "ms)\n"; std::cout.flush();
        };
        
        // 
        TestLongitude("LZ77", PerfLZ77, batch_block_size);
        TestLongitude("Zstd", PerfZstd, batch_block_size);
        TestLongitude("Snappy", PerfSnappy, batch_block_size);
        //         TestLongitude("SZ2", PerfSZ2, batch_block_size);
        //         TestLongitude("Machete", PerfMachete, batch_block_size);
        TestLongitude("SimPiece", PerfSimPiece, batch_block_size);
#ifdef USE_X86_INTRINSICS
        TestLongitude("Sprintz", PerfSprintz, batch_block_size);
#endif
        
        // （）
        TestLongitude("Deflate", PerfDeflate, streaming_block_size);
        TestLongitude("LZ4", PerfLZ4, streaming_block_size);
        TestLongitude("FPC", PerfFPC, streaming_block_size);
        TestLongitude("Gorilla", PerfGorilla, streaming_block_size);
        TestLongitude("Chimp128", PerfChimp128, streaming_block_size);
        TestLongitude("Elf", PerfElf, streaming_block_size);
        TestLongitude("SerfQt", PerfSerfQt, streaming_block_size);
        TestLongitude("SerfXOR", PerfSerfXOR, streaming_block_size);
        
        std::cout << "  ✅ \n";
        
        std::cout << "  ...\n";
        
        // lambda
        auto TestLatitude = [&](const std::string& algo_name, auto test_func, int default_block_size) {
            int actual_block_size = GetAlgorithmBlockSize(algo_name, dataset_size, default_block_size);
            std::cout << "     " << algo_name << " (block_size=" << actual_block_size;
            if (actual_block_size != default_block_size) {
                if (actual_block_size == dataset_size) {
                    std::cout << ", ";
                } else {
                    std::cout << ",  " << default_block_size;
                }
            }
            std::cout << ")...\n"; std::cout.flush();
            std::ifstream stream(config.path);
            if (!stream.is_open()) {
                std::cerr << "    ❌ \n";
                return;
            }
            std::string header;
            std::getline(stream, header);
            test_func(stream, kMaxDiffOverall, actual_block_size, config.name, 1, results_2d[algo_name].latitude_record);
            stream.close();
            std::cout << "    ✅ " << algo_name << " \n"; std::cout.flush();
        };
        
        // 
        TestLatitude("LZ77", PerfLZ77, batch_block_size);
        TestLatitude("Zstd", PerfZstd, batch_block_size);
        TestLatitude("Snappy", PerfSnappy, batch_block_size);
        //         TestLatitude("SZ2", PerfSZ2, batch_block_size);
        //         TestLatitude("Machete", PerfMachete, batch_block_size);
        TestLatitude("SimPiece", PerfSimPiece, batch_block_size);
#ifdef USE_X86_INTRINSICS
        TestLatitude("Sprintz", PerfSprintz, batch_block_size);
#endif
        
        // （）
        TestLatitude("Deflate", PerfDeflate, streaming_block_size);
        TestLatitude("LZ4", PerfLZ4, streaming_block_size);
        TestLatitude("FPC", PerfFPC, streaming_block_size);
        TestLatitude("Gorilla", PerfGorilla, streaming_block_size);
        TestLatitude("Chimp128", PerfChimp128, streaming_block_size);
        TestLatitude("Elf", PerfElf, streaming_block_size);
        TestLatitude("SerfQt", PerfSerfQt, streaming_block_size);
        TestLatitude("SerfXOR", PerfSerfXOR, streaming_block_size);
        
        std::cout << "  ✅ \n";
        
        // Simple（2D）
        std::cout << "   CoST...\n"; std::cout.flush();
        PerfRecord cost_record;
        {
            std::ifstream cost_stream(config.path);
            if (!cost_stream.is_open()) {
                std::cerr << "    ❌ : " << config.path << "\n";
            } else {
                std::string header;
                std::getline(cost_stream, header);
                // Simple  block_size
                PerfCoST(cost_stream, kMaxDiffOverall, streaming_block_size, config.name, cost_record);
                cost_stream.close();
                std::cout << "  ✅ CoST \n"; std::cout.flush();
            }
        }
        
        // （）- 
        std::cout << "  :\n";
        
        // 
        std::vector<std::string> output_order = {
            "LZ77", "Zstd", "Snappy", "SimPiece",
#ifdef USE_X86_INTRINSICS
            "Sprintz",
#endif
            "Deflate", "LZ4", "FPC", "Gorilla", "Chimp128", "Elf", "SerfQt", "SerfXOR"
        };
        
        for (const auto& method : output_order) {
            auto it = results_2d.find(method);
            if (it == results_2d.end()) continue;  // 
            
            const auto& record = it->second;
            
            // ， block_size
            bool is_batch = (method == "LZ77" || method == "Zstd" || method == "Snappy" || 
                           method == "SZ2" || method == "Machete" || method == "SimPiece" || method == "Sprintz");
            int used_block_size = is_batch ? batch_block_size : streaming_block_size;
            
            double avg_cr = record.AvgCompressionRatio(used_block_size);
            // BPP: 1D，bitsGPS
            long lon_bits = record.longitude_record.total_compressed_bits;
            long lat_bits = record.latitude_record.total_compressed_bits;
            long total_bits = lon_bits + lat_bits;
            int gps_points = record.longitude_record.total_points;  //  total_points  block_count * block_size
            double bpp = gps_points > 0 ? static_cast<double>(total_bits) / gps_points : 0;
            
            // （）
            double avg_ct_per_point = record.AvgCompressionTimePerPoint();
            double avg_dt_per_point = record.AvgDecompressionTimePerPoint();
            
            std::cout << "    " << method << ": CR=" << std::fixed << std::setprecision(2) << avg_cr 
                      << " (BPP=" << std::setprecision(2) << bpp << ")"
                      << ", CT=" << std::setprecision(4) << avg_ct_per_point << "μs"
                      << ", DT=" << std::setprecision(4) << avg_dt_per_point << "μs\n";
            
            csv_file << method << "," << config.name << "," << used_block_size << "," << kMaxDiffOverall << ","
                     << avg_cr << "," << bpp << "," << avg_ct_per_point << "," << avg_dt_per_point << "\n";
        }
        
        // Simple（2D）
        // ：Simple， CompressionRatioByPoints()
        double cost_cr = cost_record.CompressionRatioByPoints();
        double cost_bpp = cost_record.BitsPerPoint();
        // （）
        double cost_ct_per_point = cost_record.AvgCompressionTimePerPoint();
        double cost_dt_per_point = cost_record.AvgDecompressionTimePerPoint();
        
        std::cout << "    CoST: CR=" << std::fixed << std::setprecision(2) << cost_cr 
                  << " (BPP=" << std::setprecision(2) << cost_bpp << ")"
                  << ", CT=" << std::setprecision(4) << cost_ct_per_point << "μs"
                  << ", DT=" << std::setprecision(4) << cost_dt_per_point << "μs\n";
        
        // Simple （）
        csv_file << "CoST," << config.name << "," << streaming_block_size << "," << kMaxDiffOverall << ","
                 << cost_cr << "," << cost_bpp << "," << cost_ct_per_point << "," << cost_dt_per_point << "\n";
    }  // end of dataset loop
    
    csv_file.close();
    std::cout << "\n✅ : " << csv_filename.str() << "\n";
    std::cout << "====================================================================================================\n";
}

int main() {
    RunComprehensiveTest();
    return 0;
}

