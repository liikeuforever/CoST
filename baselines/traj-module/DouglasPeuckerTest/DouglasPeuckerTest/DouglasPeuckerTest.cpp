#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS  // 禁用VS的安全警告
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cstdio>
#include <set>
#include <algorithm>
#include <tuple>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// ==================== GPS点类 ====================
class GPSPoint {
public:
    int U_id;        // 轨迹ID，用于分段处理
    double lat;
    double lon;
    long long time;  // 时间戳（毫秒），使用long long避免精度损失

    GPSPoint() : U_id(0), lat(0), lon(0), time(0) {}
    GPSPoint(int uid, double latitude, double longitude, long long timestamp)
        : U_id(uid), lat(latitude), lon(longitude), time(timestamp) {}
};

// ==================== 时间解析函数 ====================
// Geolife格式: date格式: "2008/10/23", time格式: "2:53:04"
long long parseTimestampGeolife(const string& dateStr, const string& timeStr) {
    struct tm tm = {};
    int year, month, day, hour, minute, second;

    // 解析日期: "2008/10/23"
    if (sscanf(dateStr.c_str(), "%d/%d/%d", &year, &month, &day) != 3) {
        cerr << "警告: 无法解析日期格式: " << dateStr << endl;
        return 0;
    }

    // 解析时间: "2:53:04" 或 "02:53:04"
    if (sscanf(timeStr.c_str(), "%d:%d:%d", &hour, &minute, &second) != 3) {
        cerr << "警告: 无法解析时间格式: " << timeStr << endl;
        return 0;
    }

    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;
    time_t t = mktime(&tm);
    return (long long)t * 1000;
}

// WX_taxi和Trajectory格式: timestamp格式: "2020-07-18 09:07:17" 或 "2022-01-29 23:28:00"
long long parseTimestampFull(const string& timestampStr) {
    struct tm tm = {};
    int year, month, day, hour, minute, second;

    // 解析完整时间戳: "2020-07-18 09:07:17"
    if (sscanf(timestampStr.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
        cerr << "警告: 无法解析时间戳格式: " << timestampStr << endl;
        return 0;
    }

    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;
    time_t t = mktime(&tm);
    return (long long)t * 1000;
}

// ==================== 读取GPS数据 (支持多种数据集格式) ====================
// 支持的格式：
// 1. Geolife: latitude,longitude,date,time,U_id (5列)
// 2. WX_taxi: longitude,latitude,timestamp,name_id (4列)
// 3. Trajectory: longitude,latitude,timestamp,osm_way_id (4列)
vector<GPSPoint> readGPS(const string& filename, int maxPoints = -1) {
    vector<GPSPoint> points;
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "无法打开文件: " << filename << endl;
        return points;
    }

    string line;
    bool firstLine = true;
    int count = 0;
    int datasetType = -1;  // -1:未检测, 0:Geolife, 1:WX_taxi, 2:Trajectory

    while (getline(file, line)) {
        if (line.empty()) continue;

        if (firstLine) {
            firstLine = false;
            // 检测数据集类型
            string header = line;
            transform(header.begin(), header.end(), header.begin(), ::tolower);
            if (header.find("latitude") != string::npos && header.find("date") != string::npos) {
                datasetType = 0;  // Geolife格式
                cout << "检测到数据集格式: Geolife" << endl;
            }
            else if (header.find("name_id") != string::npos) {
                datasetType = 1;  // WX_taxi格式
                cout << "检测到数据集格式: WX_taxi" << endl;
            }
            else if (header.find("osm_way_id") != string::npos) {
                datasetType = 2;  // Trajectory格式
                cout << "检测到数据集格式: Trajectory (WorldTrace)" << endl;
            }
            else {
                // 尝试根据列数判断
                stringstream ss(line);
                vector<string> fields;
                string field;
                while (getline(ss, field, ',')) {
                    fields.push_back(field);
                }
                if (fields.size() == 5) {
                    datasetType = 0;  // 假设是Geolife
                    cout << "根据列数推断数据集格式: Geolife (5列)" << endl;
                }
                else if (fields.size() == 4) {
                    datasetType = 1;  // 假设是WX_taxi或Trajectory
                    cout << "根据列数推断数据集格式: WX_taxi/Trajectory (4列)" << endl;
                }
                else {
                    cerr << "警告: 无法识别数据集格式，列数: " << fields.size() << endl;
                    cerr << "表头: " << line << endl;
                }
            }
            continue;
        }

        if (maxPoints > 0 && count >= maxPoints) {
            break;
        }

        stringstream ss(line);
        string field1, field2, field3, field4, field5;

        try {
            if (datasetType == 0) {
                // Geolife格式: latitude,longitude,date,time,U_id
                if (getline(ss, field1, ',') &&
                    getline(ss, field2, ',') &&
                    getline(ss, field3, ',') &&
                    getline(ss, field4, ',') &&
                    getline(ss, field5)) {

                    double latitude = stod(field1);
                    double longitude = stod(field2);
                    long long uidLong = stoll(field5);
                    int uid = (int)uidLong;
                    long long timestamp = parseTimestampGeolife(field3, field4);

                    if (timestamp == 0) {
                        cerr << "警告: 时间戳解析失败，跳过此行: " << line << endl;
                        continue;
                    }

                    points.push_back(GPSPoint(uid, latitude, longitude, timestamp));
                    count++;
                }
            }
            else if (datasetType == 1 || datasetType == 2) {
                // WX_taxi或Trajectory格式: longitude,latitude,timestamp,uid_field
                if (getline(ss, field1, ',') &&
                    getline(ss, field2, ',') &&
                    getline(ss, field3, ',') &&
                    getline(ss, field4)) {

                    double longitude = stod(field1);  // 注意：第一个是longitude
                    double latitude = stod(field2);   // 第二个是latitude
                    long long uidLong = stoll(field4);
                    int uid = (int)uidLong;
                    long long timestamp = parseTimestampFull(field3);

                    if (timestamp == 0) {
                        cerr << "警告: 时间戳解析失败，跳过此行: " << line << endl;
                        continue;
                    }

                    points.push_back(GPSPoint(uid, latitude, longitude, timestamp));
                    count++;
                }
            }
            else {
                // 未识别的格式，尝试通用解析
                vector<string> fields;
                string field;
                while (getline(ss, field, ',')) {
                    fields.push_back(field);
                }
                if (fields.size() >= 4) {
                    // 尝试解析
                    double lat = stod(fields[fields.size() - 4]);
                    double lon = stod(fields[fields.size() - 3]);
                    long long uidLong = stoll(fields[fields.size() - 1]);
                    int uid = (int)uidLong;
                    // 尝试解析时间戳
                    long long timestamp = 0;
                    if (fields.size() == 5) {
                        timestamp = parseTimestampGeolife(fields[2], fields[3]);
                    }
                    else {
                        timestamp = parseTimestampFull(fields[2]);
                    }
                    if (timestamp > 0) {
                        points.push_back(GPSPoint(uid, lat, lon, timestamp));
                        count++;
                    }
                }
            }
        }
        catch (const exception& e) {
            cerr << "解析行时出错: " << line << " (错误: " << e.what() << ")" << endl;
        }
    }

    file.close();
    return points;
}

// ==================== 计算PED（垂直欧氏距离）====================
// 单位：度（用于算法内部判断）
double calcPED(const GPSPoint& s, const GPSPoint& m, const GPSPoint& e) {
    double A = e.lon - s.lon;
    double B = s.lat - e.lat;
    double C = e.lat * s.lon - s.lat * e.lon;

    if (A == 0 && B == 0) {
        return 0;
    }

    double ped = abs((A * m.lat + B * m.lon + C) / sqrt(A * A + B * B));
    return ped;
}

// ==================== 计算SED（同步欧氏距离）====================
// 单位：度（用于误差度量）
double calcSED(const GPSPoint& s, const GPSPoint& m, const GPSPoint& e) {
    double numerator = (double)(m.time - s.time);
    double denominator = (double)(e.time - s.time);
    double timeRatio = (denominator == 0) ? 1.0 : (numerator / denominator);

    double latEst = s.lat + (e.lat - s.lat) * timeRatio;
    double lonEst = s.lon + (e.lon - s.lon) * timeRatio;
    double latDiff = latEst - m.lat;
    double lonDiff = lonEst - m.lon;

    return sqrt(latDiff * latDiff + lonDiff * lonDiff);
}

// ==================== Douglas-Peucker算法 ====================
void douglasPeucker(const vector<GPSPoint>& points, int start, int end,
    double epsilon, vector<int>& result) {
    double dmax = 0;
    int index = start;

    // 找到距离直线最远的点
    for (int i = start + 1; i < end; i++) {
        double d = calcPED(points[start], points[i], points[end]);
        if (d > dmax) {
            index = i;
            dmax = d;
        }
    }

    // 如果最大距离大于阈值，递归细分
    if (dmax > epsilon) {
        douglasPeucker(points, start, index, epsilon, result);
        douglasPeucker(points, index, end, epsilon, result);
    }
    else {
        result.push_back(start);
        result.push_back(end);
    }
}

// ==================== 计算误差和解压缩时间 ====================
// 返回: {平均误差, 最大误差, 解压缩时间(us)}
tuple<double, double, double> computeErrorsAndDecompressionTime(
    const vector<int>& keptIndices,
    const vector<GPSPoint>& points,
    double conversionFactor) {

    double totalError = 0;
    double maxError = 0;
    int count = 0;

    // 计时解压缩过程
    auto decompressStart = chrono::high_resolution_clock::now();

    for (size_t i = 0; i < keptIndices.size() - 1; i++) {
        int start = keptIndices[i];
        int end = keptIndices[i + 1];

        for (int j = start; j <= end; j++) {
            // 解压缩：根据保留的点计算估计位置（线性插值）
            // 使用SED计算同步欧氏距离作为误差

            double errorDeg = calcSED(points[start], points[j], points[end]);
            double errorMeters = errorDeg * conversionFactor;
            totalError += errorMeters;
            if (errorMeters > maxError) {
                maxError = errorMeters;
            }
            count++;
        }
    }

    auto decompressEnd = chrono::high_resolution_clock::now();
    double decompressTimeUs = chrono::duration<double, micro>(decompressEnd - decompressStart).count();

    double avgError = (count > 0) ? totalError / count : 0;
    return make_tuple(avgError, maxError, decompressTimeUs);
}

// ==================== 主函数 ====================
int main(int argc, char* argv[]) {
    string filename;
    double epsilonMeters;
    string saveFilename;
    int maxPoints = -1;

    if (argc < 3) {
        // Default to relative path (should be run from traj-module directory)
        filename = "data_set/Geolife_100k_with_id.csv";
        epsilonMeters = 1.0;
        saveFilename = "outputDP.txt";  // Use relative path
        maxPoints = -1;  // -1表示读取全部数据（100k个点）
    }
    else {
        filename = argv[1];
        epsilonMeters = stod(argv[2]);
        saveFilename = argv[3];
        if (argc >= 5) {
            maxPoints = atoi(argv[4]);
        }
    }

    cout << "\n========================================" << endl;
    cout << "Douglas-Peucker 轨迹压缩算法" << endl;
    cout << "========================================" << endl;
    cout << "读取数据文件: " << filename << endl;
    if (maxPoints > 0) {
        cout << "限制读取: 前 " << maxPoints << " 个点" << endl;
    }
    else {
        cout << "读取模式: 全部数据" << endl;
    }

    // 1. 读取轨迹数据
    vector<GPSPoint> points = readGPS(filename, maxPoints);
    size_t originalCount = points.size();

    if (originalCount == 0) {
        cout << "未读取到任何GPS数据！" << endl;
        return 1;
    }
    cout << "读取到 " << originalCount << " 个GPS点." << endl;

    // 2. 按U_id分组轨迹段
    vector<vector<GPSPoint>> trajectorySegments;
    vector<int> segmentStartIndices;  // 记录每个段在全局points中的起始索引

    if (points.size() > 0) {
        int currentUid = points[0].U_id;
        int segmentStart = 0;

        for (size_t i = 1; i < points.size(); i++) {
            if (points[i].U_id != currentUid) {
                // U_id改变，保存当前段
                vector<GPSPoint> segment(points.begin() + segmentStart, points.begin() + i);
                trajectorySegments.push_back(segment);
                segmentStartIndices.push_back(segmentStart);

                // 开始新段
                currentUid = points[i].U_id;
                segmentStart = (int)i;
            }
        }
        // 保存最后一段
        vector<GPSPoint> lastSegment(points.begin() + segmentStart, points.end());
        trajectorySegments.push_back(lastSegment);
        segmentStartIndices.push_back(segmentStart);
    }

    cout << "检测到 " << trajectorySegments.size() << " 个轨迹段." << endl;

    // 3. 计算平均纬度和转换系数
    double sumLat = 0;
    for (const auto& p : points) {
        sumLat += p.lat;
    }
    double avgLat = sumLat / (double)originalCount;
    double metersPerDegLat = 111320;
    double metersPerDegLon = 111320 * cos(avgLat * M_PI / 180.0);
    double conversionFactor = (metersPerDegLat + metersPerDegLon) / 2.0;
    double epsilonDegrees = epsilonMeters / conversionFactor;

    cout << "Epsilon阈值: " << epsilonMeters << " 米 (约 "
        << scientific << epsilonDegrees << " 度)" << endl;
    cout << fixed;

    // 4. 对每个轨迹段分别进行压缩和误差计算
    auto startTime = chrono::high_resolution_clock::now();

    vector<int> allKeptIndices;  // 所有轨迹段保留的索引
    double totalErrorSum = 0;     // 所有段的误差总和
    double globalMaxError = 0;    // 全局最大误差
    int totalErrorCount = 0;      // 所有段的误差计算点数
    double totalDecompressTimeUs = 0;  // 所有段的解压缩时间总和

    for (size_t segIdx = 0; segIdx < trajectorySegments.size(); segIdx++) {
        const vector<GPSPoint>& segment = trajectorySegments[segIdx];
        int startOffset = segmentStartIndices[segIdx];

        if (segment.size() < 2) {
            // 如果轨迹段点数少于2，全部保留
            for (size_t i = 0; i < segment.size(); i++) {
                allKeptIndices.push_back((int)(startOffset + i));
            }
            continue;
        }

        // 对当前轨迹段进行压缩
        vector<int> segmentKeptIndices;
        douglasPeucker(segment, 0, (int)segment.size() - 1, epsilonDegrees, segmentKeptIndices);

        // 转换为全局索引
        for (size_t i = 0; i < segmentKeptIndices.size(); i++) {
            segmentKeptIndices[i] += startOffset;
        }

        // 计算当前段的误差（使用全局索引）
        double segAvgError, segMaxError, segDecompressTimeUs;
        tie(segAvgError, segMaxError, segDecompressTimeUs) =
            computeErrorsAndDecompressionTime(segmentKeptIndices, points, conversionFactor);

        // 累计误差统计
        int segErrorCount = 0;
        for (size_t i = 0; i < segmentKeptIndices.size() - 1; i++) {
            int start = segmentKeptIndices[i];
            int end = segmentKeptIndices[i + 1];
            segErrorCount += max(0, end - start - 1);
        }

        totalErrorSum += segAvgError * segErrorCount;
        totalErrorCount += segErrorCount;
        if (segMaxError > globalMaxError) {
            globalMaxError = segMaxError;
        }
        totalDecompressTimeUs += segDecompressTimeUs;

        // 合并保留的索引
        allKeptIndices.insert(allKeptIndices.end(),
            segmentKeptIndices.begin(), segmentKeptIndices.end());
    }

    auto endTime = chrono::high_resolution_clock::now();
    double totalTimeUs = chrono::duration<double, micro>(endTime - startTime).count();

    // 5. 去重并排序所有保留的索引
    set<int> cmpSet(allKeptIndices.begin(), allKeptIndices.end());
    vector<int> cmpIndexList(cmpSet.begin(), cmpSet.end());
    sort(cmpIndexList.begin(), cmpIndexList.end());

    // 6. 计算总体统计指标
    double globalAvgError = (totalErrorCount > 0) ? totalErrorSum / totalErrorCount : 0;
    double compressionRatio = (double)cmpIndexList.size() / (double)originalCount;
    double avgTimePerPointUs = totalTimeUs / (double)originalCount;
    double avgDecompressTimePerPointUs = totalDecompressTimeUs / (double)originalCount;

    // 6. 写入输出文件
    ofstream outFile(saveFilename);
    if (outFile.is_open()) {
        outFile << fixed << setprecision(6);
        for (int idx : cmpIndexList) {
            GPSPoint p = points[idx];
            outFile << p.lat << " " << p.lon << " " << p.time << endl;
        }
        outFile << "\n========================================\n";
        outFile << "Douglas-Peucker Performance Statistics\n";
        outFile << "========================================\n";
        outFile << "Original Points: " << originalCount << endl;
        outFile << "Compressed Points: " << cmpIndexList.size() << endl;
        outFile << "Compression Ratio: " << compressionRatio * 100 << "%" << endl;
        outFile << "\n--- Compression Performance ---\n";
        outFile << "Total Time: " << totalTimeUs << " us" << endl;
        outFile << "Average Time per Point: " << avgTimePerPointUs << " us/point" << endl;
        outFile << "\n--- Decompression Performance ---\n";
        outFile << "Total Time: " << totalDecompressTimeUs << " us" << endl;
        outFile << "Average Time per Point: " << avgDecompressTimePerPointUs << " us/point" << endl;
        outFile << "\n--- Accuracy (Distance in meters) ---\n";
        outFile << "Error Threshold (epsilon): " << epsilonMeters << " m" << endl;
        outFile << "Average Error: " << globalAvgError << " m" << endl;
        outFile << "Maximum Error: " << globalMaxError << " m" << endl;
        outFile << "\n--- Trajectory Segments ---\n";
        outFile << "Number of Segments: " << trajectorySegments.size() << endl;
        outFile.close();
    }

    // 7. 控制台输出
    cout << "\n========================================" << endl;
    cout << "Douglas-Peucker 压缩结果" << endl;
    cout << "========================================" << endl;
    cout << setprecision(6);
    cout << "\n--- Data ---" << endl;
    cout << "Original Points: " << originalCount << endl;
    cout << "Compressed Points: " << cmpIndexList.size() << endl;
    cout << "Compression Ratio: " << compressionRatio * 100 << "%" << endl;
    cout << "\n--- Compression Performance ---" << endl;
    cout << "Total Time: " << totalTimeUs << " us" << endl;
    cout << "Average Time per Point: " << avgTimePerPointUs << " us/point" << endl;
    cout << "\n--- Decompression Performance ---" << endl;
    cout << "Total Time: " << totalDecompressTimeUs << " us" << endl;
    cout << "Average Time per Point: " << avgDecompressTimePerPointUs << " us/point" << endl;
    cout << "\n--- Accuracy (Distance in meters) ---" << endl;
    cout << "Error Threshold (epsilon): " << epsilonMeters << " m" << endl;
    cout << "Average Error: " << globalAvgError << " m" << endl;
    cout << "Maximum Error: " << globalMaxError << " m" << endl;
    cout << "\n--- Trajectory Segments ---" << endl;
    cout << "Number of Segments: " << trajectorySegments.size() << endl;
    cout << "========================================\n" << endl;

    return 0;
}

