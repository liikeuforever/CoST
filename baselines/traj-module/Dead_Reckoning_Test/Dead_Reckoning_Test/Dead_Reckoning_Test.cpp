#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cstdio>
#include <tuple>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// ==================== GPS Point Structure ====================
struct Point {
    int U_id;        // 轨迹ID，用于分段处理
    double lat;
    double lon;
    long long time;  // 时间戳（毫秒）

    Point() : U_id(0), lat(0), lon(0), time(0) {}
    Point(int uid, double latitude, double longitude, long long timestamp)
        : U_id(uid), lat(latitude), lon(longitude), time(timestamp) {}
};

// ==================== Time Parsing Functions ====================
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

// ==================== Read GPS Data (支持多种数据集格式) ====================
// 支持的格式：
// 1. Geolife: latitude,longitude,date,time,U_id (5列)
// 2. WX_taxi: longitude,latitude,timestamp,name_id (4列)
// 3. Trajectory: longitude,latitude,timestamp,osm_way_id (4列)
vector<Point> readGPS(const string& filename, int maxPoints = -1) {
    vector<Point> points;
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

        if (maxPoints > 0 && count >= maxPoints) break;

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

                    points.push_back(Point(uid, latitude, longitude, timestamp));
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

                    points.push_back(Point(uid, latitude, longitude, timestamp));
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
                        points.push_back(Point(uid, lat, lon, timestamp));
                        count++;
                    }
                }
            }
        }
        catch (const exception& e) {
            cerr << "Error parsing line: " << line << " (错误: " << e.what() << ")" << endl;
        }
    }

    file.close();
    return points;
}

// ==================== Calculate Distance (in degrees) ====================
double calcDistance(const Point& a, const Point& b) {
    return sqrt(pow(a.lat - b.lat, 2) + pow(a.lon - b.lon, 2));
}

// ==================== Calculate Angle ====================
double calcAngle(const Point& a, const Point& b) {
    double lat_diff = b.lat - a.lat;
    double lon_diff = b.lon - a.lon;
    return atan2(lon_diff, lat_diff);
}

// ==================== Calculate SED (Synchronized Euclidean Distance) ====================
double calcSED(const Point& s, const Point& m, const Point& e) {
    double numerator = (double)(m.time - s.time);
    double denominator = (double)(e.time - s.time);
    double timeRatio = (denominator == 0) ? 1.0 : (numerator / denominator);

    double latEst = s.lat + (e.lat - s.lat) * timeRatio;
    double lonEst = s.lon + (e.lon - s.lon) * timeRatio;
    double latDiff = latEst - m.lat;
    double lonDiff = lonEst - m.lon;

    return sqrt(latDiff * latDiff + lonDiff * lonDiff);
}

// ==================== Dead Reckoning Algorithm ====================
vector<int> DeadReckoning(const vector<Point>& points, double epsilon) {
    int n = (int)points.size();
    if (n < 2) {
        vector<int> result;
        for (int i = 0; i < n; i++) {
            result.push_back(i);
        }
        return result;
    }

    double max_d = 0;
    int start_idx = 0;
    vector<int> simplifindex;
    simplifindex.push_back(0);

    // 计算距离和角度
    vector<double> distances;
    vector<double> angles;
    for (int i = 1; i < n; i++) {
        distances.push_back(calcDistance(points[i - 1], points[i]));
        angles.push_back(calcAngle(points[i - 1], points[i]));
    }

    for (int i = 2; i < n; i++) {
        max_d += fabs(distances[i - 1] * sin(angles[i - 1] - angles[start_idx]));
        if (fabs(max_d) > epsilon) {
            max_d = 0;
            simplifindex.push_back(i - 1);
            start_idx = i - 1;
        }
    }

    if (simplifindex[simplifindex.size() - 1] != n - 1) {
        simplifindex.push_back(n - 1);
    }

    return simplifindex;
}

// ==================== Compute Errors and Decompression Time ====================
// 返回: {平均误差, 最大误差, 解压缩时间(us)}
tuple<double, double, double> computeErrorsAndDecompressionTime(
    const vector<int>& simplifiedIndices,
    const vector<Point>& points,
    double conversionFactor) {

    double totalError = 0;
    double maxError = 0;
    int count = 0;

    // 如果保留点少于2个，误差为0
    if (simplifiedIndices.size() < 2) {
        return make_tuple(0.0, 0.0, 0.0);
    }

    // 计时解压缩过程
    auto decompressStart = chrono::high_resolution_clock::now();

    for (size_t i = 0; i < simplifiedIndices.size() - 1; i++) {
        int start = simplifiedIndices[i];
        int end = simplifiedIndices[i + 1];

        // 确保索引有效
        if (start < 0 || end >= (int)points.size() || start >= end) {
            continue;
        }

        // 只计算中间点的误差（不包括start和end点）
        for (int j = start + 1; j < end; j++) {
            // 确保索引有效
            if (j < 0 || j >= (int)points.size()) {
                continue;
            }

            double errDeg = calcSED(points[start], points[j], points[end]);
            double errMeters = errDeg * conversionFactor;

            // 检查异常大的误差
            if (errMeters > 100000) {  // 超过100公里
                cerr << "警告: 异常大的误差: " << errMeters << "m, 段["
                    << start << "->" << end << "], 点" << j << endl;
            }

            totalError += errMeters;
            if (errMeters > maxError) {
                maxError = errMeters;
            }
            count++;
        }
    }

    auto decompressEnd = chrono::high_resolution_clock::now();
    double decompressTimeUs = chrono::duration<double, micro>(decompressEnd - decompressStart).count();

    double avgError = (count > 0) ? totalError / count : 0;
    return make_tuple(avgError, maxError, decompressTimeUs);
}

// ==================== Main Function ====================
int main(int argc, char* argv[]) {
    string filename;
    double epsilonMeters;
    string saveFilename;
    int maxPoints = -1;

    if (argc < 3) {
        // Default to relative path (should be run from traj-module directory)
        filename = "data_set/Geolife_100k_with_id.csv";
        epsilonMeters = 1.0;  // 默认阈值（米）
        saveFilename = "outputDR.txt";  // Use relative path
        maxPoints = -1;
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
    cout << "Dead Reckoning Trajectory Compression" << endl;
    cout << "========================================" << endl;
    cout << "Reading data file: " << filename << endl;
    if (maxPoints > 0) {
        cout << "Limited to: " << maxPoints << " points" << endl;
    }
    else {
        cout << "Reading mode: All data" << endl;
    }

    // 1. Read trajectory data
    vector<Point> points = readGPS(filename, maxPoints);
    size_t originalCount = points.size();

    if (originalCount == 0) {
        cout << "No GPS data loaded!" << endl;
        return 1;
    }
    cout << "Loaded " << originalCount << " GPS points." << endl;

    // 2. Group trajectory segments by U_id
    vector<vector<Point>> trajectorySegments;
    vector<int> segmentStartIndices;

    if (points.size() > 0) {
        int currentUid = points[0].U_id;
        int segmentStart = 0;

        for (size_t i = 1; i < points.size(); i++) {
            if (points[i].U_id != currentUid) {
                vector<Point> segment(points.begin() + segmentStart, points.begin() + i);
                trajectorySegments.push_back(segment);
                segmentStartIndices.push_back(segmentStart);
                currentUid = points[i].U_id;
                segmentStart = (int)i;
            }
        }
        vector<Point> lastSegment(points.begin() + segmentStart, points.end());
        trajectorySegments.push_back(lastSegment);
        segmentStartIndices.push_back(segmentStart);

        cout << "Detected " << trajectorySegments.size() << " trajectory segments." << endl;
    }
    else {
        cout << "Detected 0 trajectory segments." << endl;
    }

    // 3. Calculate average latitude and conversion factor
    double sumLat = 0;
    for (const auto& p : points) {
        sumLat += p.lat;
    }
    double avgLat = sumLat / (double)originalCount;
    double metersPerDegLat = 111320;
    double metersPerDegLon = 111320 * cos(avgLat * M_PI / 180.0);
    double conversionFactor = (metersPerDegLat + metersPerDegLon) / 2.0;
    double epsilonDegrees = epsilonMeters / conversionFactor;

    cout << "Epsilon threshold: " << epsilonMeters << " meters (" << epsilonDegrees << " degrees)" << endl;

    // 4. Execute Dead Reckoning algorithm for each segment with timing
    auto startTime = chrono::high_resolution_clock::now();

    vector<int> allKeptIndices;
    double totalErrorSum = 0;
    double globalMaxError = 0;
    int totalErrorCount = 0;
    double totalDecompressTimeUs = 0;  // 所有段的解压缩时间总和

    for (size_t segIdx = 0; segIdx < trajectorySegments.size(); segIdx++) {
        const vector<Point>& segment = trajectorySegments[segIdx];
        int startOffset = segmentStartIndices[segIdx];

        if (segment.size() < 2) {
            for (size_t i = 0; i < segment.size(); i++) {
                allKeptIndices.push_back((int)(startOffset + i));
            }
            continue;
        }

        vector<int> segmentKeptIndices = DeadReckoning(segment, epsilonDegrees);

        // 转换为全局索引
        for (size_t i = 0; i < segmentKeptIndices.size(); i++) {
            segmentKeptIndices[i] += startOffset;
        }

        // 计算当前段的误差（使用段内相对索引和段内点）
        // 先获取段内相对索引（用于误差计算）
        vector<int> segmentRelativeIndices;
        for (int idx : segmentKeptIndices) {
            segmentRelativeIndices.push_back(idx - startOffset);
        }

        // 使用段内相对索引和段内点计算误差
        double segAvgError, segMaxError, segDecompressTimeUs;
        tie(segAvgError, segMaxError, segDecompressTimeUs) =
            computeErrorsAndDecompressionTime(segmentRelativeIndices, segment, conversionFactor);

        // 计算段内实际误差计算的点数（不包括起点和终点）
        int segErrorCount = 0;
        for (size_t i = 0; i < segmentRelativeIndices.size() - 1; i++) {
            int start = segmentRelativeIndices[i];
            int end = segmentRelativeIndices[i + 1];
            segErrorCount += max(0, end - start - 1);
        }

        // 如果段内没有中间点（只有起点和终点），误差为0
        if (segErrorCount == 0) {
            segAvgError = 0;
            segMaxError = 0;
        }

        totalErrorSum += segAvgError * segErrorCount;
        totalErrorCount += segErrorCount;
        if (segMaxError > globalMaxError) {
            globalMaxError = segMaxError;
        }
        totalDecompressTimeUs += segDecompressTimeUs;

        allKeptIndices.insert(allKeptIndices.end(),
            segmentKeptIndices.begin(), segmentKeptIndices.end());
    }

    auto endTime = chrono::high_resolution_clock::now();
    double totalTimeUs = chrono::duration<double, micro>(endTime - startTime).count();

    // 5. Statistics
    set<int> cmpSet(allKeptIndices.begin(), allKeptIndices.end());
    vector<int> simplifiedIndices(cmpSet.begin(), cmpSet.end());
    sort(simplifiedIndices.begin(), simplifiedIndices.end());

    int simplifiedCount = simplifiedIndices.size();
    double globalAvgError = (totalErrorCount > 0) ? totalErrorSum / totalErrorCount : 0;
    double compressionRatio = (double)simplifiedCount / (double)originalCount;
    double avgTimePerPointUs = totalTimeUs / (double)originalCount;
    double avgDecompressTimePerPointUs = totalDecompressTimeUs / (double)originalCount;

    // 6. Write to output file
    ofstream outFile(saveFilename);
    if (outFile.is_open()) {
        outFile << fixed << setprecision(6);
        for (int idx : simplifiedIndices) {
            Point p = points[idx];
            outFile << p.lat << " " << p.lon << " " << p.time << endl;
        }
        outFile << "\n========================================\n";
        outFile << "Dead Reckoning Performance Statistics\n";
        outFile << "========================================\n";
        outFile << "Original Points: " << originalCount << endl;
        outFile << "Compressed Points: " << simplifiedCount << endl;
        outFile << "Compression Ratio: " << compressionRatio * 100 << "%" << endl;
        outFile << "\n--- Compression Performance ---\n";
        outFile << "Total Time: " << totalTimeUs << " us" << endl;
        outFile << "Average Time per Point: " << avgTimePerPointUs << " us/point" << endl;
        outFile << "\n--- Decompression Performance ---\n";
        outFile << "Total Time: " << totalDecompressTimeUs << " us" << endl;
        outFile << "Average Time per Point: " << avgDecompressTimePerPointUs << " us/point" << endl;
        outFile << "\n--- Accuracy ---\n";
        outFile << "Error Threshold (epsilon): " << epsilonMeters << " m" << endl;
        outFile << "Average Error: " << globalAvgError << " m" << endl;
        outFile << "Maximum Error: " << globalMaxError << " m" << endl;
        outFile << "\n--- Trajectory Segments ---\n";
        outFile << "Number of Segments: " << trajectorySegments.size() << endl;
        outFile.close();
    }

    // 7. Console output
    cout << "\n========================================" << endl;
    cout << "Dead Reckoning Compression Results" << endl;
    cout << "========================================" << endl;
    cout << setprecision(6);
    cout << "\n--- Data ---" << endl;
    cout << "Original Points: " << originalCount << endl;
    cout << "Compressed Points: " << simplifiedCount << endl;
    cout << "Compression Ratio: " << compressionRatio * 100 << "%" << endl;
    cout << "\n--- Compression Performance ---" << endl;
    cout << "Total Time: " << totalTimeUs << " us" << endl;
    cout << "Average Time per Point: " << avgTimePerPointUs << " us/point" << endl;
    cout << "\n--- Decompression Performance ---" << endl;
    cout << "Total Time: " << totalDecompressTimeUs << " us" << endl;
    cout << "Average Time per Point: " << avgDecompressTimePerPointUs << " us/point" << endl;
    cout << "\n--- Accuracy ---" << endl;
    cout << "Error Threshold (epsilon): " << epsilonMeters << " m" << endl;
    cout << "Average Error: " << globalAvgError << " m" << endl;
    cout << "Maximum Error: " << globalMaxError << " m" << endl;
    cout << "\n--- Trajectory Segments ---" << endl;
    cout << "Number of Segments: " << trajectorySegments.size() << endl;
    cout << "========================================\n" << endl;

    return 0;
}
