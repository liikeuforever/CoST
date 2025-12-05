#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
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
#include <algorithm>
#include <limits>
#include <tuple>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// ==================== GPS Point Class ====================
class GPSPoint {
public:
    int U_id;        // 轨迹ID，用于分段处理
    int index;       // 算法内部索引
    double lat;
    double lon;
    long long time;

    GPSPoint() : U_id(0), index(0), lat(0), lon(0), time(0) {}
    GPSPoint(int uid, int idx, double latitude, double longitude, long long timestamp)
        : U_id(uid), index(idx), lat(latitude), lon(longitude), time(timestamp) {}
};

// ==================== GPS Point with SED (for SQUISH-E) ====================
class GPSPointWithSED {
public:
    double priority;
    double pi;
    GPSPoint point;

    GPSPointWithSED(double prio, double piVal, const GPSPoint& p)
        : priority(prio), pi(piVal), point(p) {}
};

// ==================== Time Parsing Function ====================
// date格式: "2008/10/23", time格式: "2:53:04"
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
vector<GPSPoint> readGPS(const string& filename, int maxPoints = -1) {
    vector<GPSPoint> points;
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Cannot open file: " << filename << endl;
        return points;
    }

    string line;
    bool firstLine = true;
    int count = 0;
    int index = 0;
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

                    points.push_back(GPSPoint(uid, index, latitude, longitude, timestamp));
                    count++;
                    index++;
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

                    points.push_back(GPSPoint(uid, index, latitude, longitude, timestamp));
                    count++;
                    index++;
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
                        points.push_back(GPSPoint(uid, index, lat, lon, timestamp));
                        count++;
                        index++;
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

// ==================== Calculate SED (in degrees) ====================
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

// ==================== Adjust Priority ====================
void adjustPriority(vector<GPSPointWithSED>& Q, int preIndex, int qIndex, int succIndex) {
    if (qIndex <= 0 || qIndex >= (int)Q.size() - 1) return;

    double p = Q[qIndex].pi + calcSED(Q[preIndex].point, Q[qIndex].point, Q[succIndex].point);
    Q[qIndex].priority = p;
}

// ==================== Find Minimum Priority ====================
int findMinPriority(const vector<GPSPointWithSED>& Q) {
    if (Q.size() < 3) return 1;

    int minIndex = 1;
    double minPriority = Q[1].priority;

    for (int k = 2; k < (int)Q.size() - 1; k++) {
        if (Q[k].priority < minPriority) {
            minPriority = Q[k].priority;
            minIndex = k;
        }
    }

    return minIndex;
}

// ==================== Reduce ====================
void reduce(vector<GPSPointWithSED>& Q, int minIndex, double minP) {
    if (minIndex - 1 >= 0) {
        Q[minIndex - 1].pi = max(minP, Q[minIndex - 1].pi);
    }
    if (minIndex + 1 < (int)Q.size()) {
        Q[minIndex + 1].pi = max(minP, Q[minIndex + 1].pi);
    }

    if (minIndex - 2 >= 0 && minIndex - 1 < (int)Q.size() && minIndex + 1 < (int)Q.size()) {
        adjustPriority(Q, minIndex - 2, minIndex - 1, minIndex + 1);
    }
    if (minIndex - 1 >= 0 && minIndex + 1 < (int)Q.size() && minIndex + 2 < (int)Q.size()) {
        adjustPriority(Q, minIndex - 1, minIndex + 1, minIndex + 2);
    }

    Q.erase(Q.begin() + minIndex);
}

// ==================== SQUISH-E Algorithm ====================
vector<GPSPointWithSED> SQUISH_E(const vector<GPSPoint>& points, int cmpRatio, double sedError) {
    vector<GPSPointWithSED> Q;
    int capacity = 4;
    int i = 0;

    while (i < (int)points.size()) {
        if ((i / cmpRatio) >= capacity) {
            capacity++;
        }

        Q.push_back(GPSPointWithSED(numeric_limits<double>::max(), 0, points[i]));

        if (i > 0 && (int)Q.size() >= 3) {
            adjustPriority(Q, Q.size() - 3, Q.size() - 2, Q.size() - 1);
        }

        if ((int)Q.size() == capacity) {
            int minIndex = findMinPriority(Q);
            double minP = Q[minIndex].priority;
            reduce(Q, minIndex, minP);
        }

        i++;
    }

    // Final reduction based on sed_error threshold
    if (Q.size() >= 3) {
        int minIndex = findMinPriority(Q);
        double minP = Q[minIndex].priority;

        while (minP <= sedError && (int)Q.size() > 2) {
            reduce(Q, minIndex, minP);
            if ((int)Q.size() < 3) break;
            minIndex = findMinPriority(Q);
            minP = Q[minIndex].priority;
        }
    }

    return Q;
}

// ==================== Compute Errors and Decompression Time ====================
// 返回: {平均误差, 最大误差, 解压缩时间(us)}
// Q: 压缩后的点（包含全局索引）
// pts: 段内的原始点（索引从0开始）
tuple<double, double, double> computeErrorsAndDecompressionTime(
    const vector<GPSPointWithSED>& Q,
    const vector<GPSPoint>& pts,
    double conversionFactor) {

    // 如果压缩后的点少于2个，误差为0
    if (Q.size() < 2) {
        return make_tuple(0.0, 0.0, 0.0);
    }

    // 找到Q中的点在pts中的位置（通过比较坐标和时间戳）
    vector<int> idxList;
    for (const auto& gps : Q) {
        // 在pts中查找匹配的点
        int foundIdx = -1;
        for (size_t k = 0; k < pts.size(); k++) {
            // 通过比较坐标和时间戳来匹配点
            if (abs(pts[k].lat - gps.point.lat) < 1e-9 &&
                abs(pts[k].lon - gps.point.lon) < 1e-9 &&
                pts[k].time == gps.point.time) {
                foundIdx = (int)k;
                break;
            }
        }
        if (foundIdx >= 0) {
            idxList.push_back(foundIdx);
        }
    }

    if (idxList.size() < 2) {
        return make_tuple(0.0, 0.0, 0.0);
    }

    sort(idxList.begin(), idxList.end());

    double totalError = 0;
    double maxError = 0;
    int count = 0;

    // 计时解压缩过程
    auto decompressStart = chrono::high_resolution_clock::now();

    for (size_t i = 0; i < idxList.size() - 1; i++) {
        int start = idxList[i];
        int end = idxList[i + 1];

        // 确保索引有效
        if (start < 0 || end >= (int)pts.size() || start >= end) {
            continue;
        }

        // 只计算中间点的误差（不包括start和end点）
        for (int j = start + 1; j < end; j++) {
            // 确保索引有效
            if (j < 0 || j >= (int)pts.size()) {
                continue;
            }

            // 解压缩：根据保留的点计算估计位置（SED计算中包含线性插值）
            double errDeg = calcSED(pts[start], pts[j], pts[end]);
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
    double ratio;
    double sedMeters;
    string saveFilename;
    int maxPoints = -1;

    if (argc < 4) {
        // Default to relative path (should be run from traj-module directory)
        filename = "data_set/Geolife_100k_with_id.csv";
        ratio = 1.0;
        sedMeters = 1.0;
        saveFilename = "outputSQE.txt";  // Use relative path
        maxPoints = -1;
    }
    else {
        filename = argv[1];
        ratio = stod(argv[2]);
        sedMeters = stod(argv[3]);
        saveFilename = argv[4];
        if (argc >= 6) {
            maxPoints = atoi(argv[5]);
        }
    }

    cout << "\n========================================" << endl;
    cout << "SQUISH-E Trajectory Compression" << endl;
    cout << "========================================" << endl;
    cout << "Reading data file: " << filename << endl;
    if (maxPoints > 0) {
        cout << "Limited to: " << maxPoints << " points" << endl;
    }
    else {
        cout << "Reading mode: All data" << endl;
    }

    // 1. Read trajectory data
    vector<GPSPoint> points = readGPS(filename, maxPoints);
    size_t originalCount = points.size();

    if (originalCount == 0) {
        cout << "No GPS data loaded!" << endl;
        return 1;
    }
    cout << "Loaded " << originalCount << " GPS points." << endl;

    // 2. Group trajectory segments by U_id
    vector<vector<GPSPoint>> trajectorySegments;
    vector<int> segmentStartIndices;

    if (points.size() > 0) {
        int currentUid = points[0].U_id;
        int segmentStart = 0;

        for (size_t i = 1; i < points.size(); i++) {
            if (points[i].U_id != currentUid) {
                vector<GPSPoint> segment(points.begin() + segmentStart, points.begin() + i);
                trajectorySegments.push_back(segment);
                segmentStartIndices.push_back(segmentStart);
                currentUid = points[i].U_id;
                segmentStart = (int)i;
            }
        }
        vector<GPSPoint> lastSegment(points.begin() + segmentStart, points.end());
        trajectorySegments.push_back(lastSegment);
        segmentStartIndices.push_back(segmentStart);
    }

    cout << "Detected " << trajectorySegments.size() << " trajectory segments." << endl;

    // 3. Calculate average latitude and conversion factor
    double sumLat = 0;
    for (const auto& p : points) {
        sumLat += p.lat;
    }
    double avgLat = sumLat / (double)originalCount;
    double metersPerDegLat = 111320;
    double metersPerDegLon = 111320 * cos(avgLat * M_PI / 180.0);
    double conversionFactor = (metersPerDegLat + metersPerDegLon) / 2.0;
    double sedDegrees = sedMeters / conversionFactor;

    cout << "Ratio: " << ratio << ", SED threshold: " << sedMeters << " meters" << endl;

    // 4. Execute SQUISH-E algorithm for each segment with timing
    auto startTime = chrono::high_resolution_clock::now();

    vector<GPSPointWithSED> allCompressedPoints;
    double totalErrorSum = 0;
    double globalMaxError = 0;
    int totalErrorCount = 0;
    double totalDecompressTimeUs = 0;

    for (size_t segIdx = 0; segIdx < trajectorySegments.size(); segIdx++) {
        const vector<GPSPoint>& segment = trajectorySegments[segIdx];

        if (segment.size() < 2) {
            for (size_t i = 0; i < segment.size(); i++) {
                allCompressedPoints.push_back(GPSPointWithSED(0, 0, segment[i]));
            }
            continue;
        }

        vector<GPSPointWithSED> segmentCompressed = SQUISH_E(segment, (int)ratio, sedDegrees);

        double segAvgError, segMaxError, segDecompressTimeUs;
        tie(segAvgError, segMaxError, segDecompressTimeUs) =
            computeErrorsAndDecompressionTime(segmentCompressed, segment, conversionFactor);

        // 计算段内实际误差计算的点数（不包括起点和终点）
        int segErrorCount = 0;
        // 找到压缩后的点在段内的索引
        vector<int> segIdxList;
        for (const auto& gps : segmentCompressed) {
            for (size_t k = 0; k < segment.size(); k++) {
                if (abs(segment[k].lat - gps.point.lat) < 1e-9 &&
                    abs(segment[k].lon - gps.point.lon) < 1e-9 &&
                    segment[k].time == gps.point.time) {
                    segIdxList.push_back((int)k);
                    break;
                }
            }
        }
        sort(segIdxList.begin(), segIdxList.end());
        for (size_t i = 0; i < segIdxList.size() - 1; i++) {
            int start = segIdxList[i];
            int end = segIdxList[i + 1];
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

        allCompressedPoints.insert(allCompressedPoints.end(),
            segmentCompressed.begin(), segmentCompressed.end());
    }

    auto endTime = chrono::high_resolution_clock::now();
    double totalTimeUs = chrono::duration<double, micro>(endTime - startTime).count();

    // 5. Statistics
    int simplifiedCount = allCompressedPoints.size();
    double compressionRatio = (double)simplifiedCount / (double)originalCount;
    double avgTimePerPointUs = totalTimeUs / (double)originalCount;
    double globalAvgError = (totalErrorCount > 0) ? totalErrorSum / totalErrorCount : 0;
    double avgDecompressTimePerPointUs = totalDecompressTimeUs / (double)originalCount;

    // 5. Write to output file
    ofstream outFile(saveFilename);
    if (outFile.is_open()) {
        outFile << fixed << setprecision(6);
        for (const auto& gps : allCompressedPoints) {
            outFile << gps.point.lat << " " << gps.point.lon << " " << gps.point.time << endl;
        }
        outFile << "\n========================================\n";
        outFile << "SQUISH-E Performance Statistics\n";
        outFile << "========================================\n";
        outFile << "Original Points: " << originalCount << endl;
        outFile << "Simplified Points: " << simplifiedCount << endl;
        outFile << "Compression Ratio: " << compressionRatio * 100 << "%" << endl;
        outFile << "\n--- Compression Performance ---\n";
        outFile << "Total Time: " << totalTimeUs << " us" << endl;
        outFile << "Average Time per Point: " << avgTimePerPointUs << " us/point" << endl;
        outFile << "\n--- Decompression Performance ---\n";
        outFile << "Total Time: " << totalDecompressTimeUs << " us" << endl;
        outFile << "Average Time per Point: " << avgDecompressTimePerPointUs << " us/point" << endl;
        outFile << "\n--- Accuracy ---\n";
        outFile << "SED Threshold: " << sedMeters << " m" << endl;
        outFile << "Average Error: " << globalAvgError << " m" << endl;
        outFile << "Maximum Error: " << globalMaxError << " m" << endl;
        outFile << "\n--- Trajectory Segments ---\n";
        outFile << "Number of Segments: " << trajectorySegments.size() << endl;
        outFile.close();
    }

    // 6. Console output
    cout << "\n========================================" << endl;
    cout << "SQUISH-E Compression Results" << endl;
    cout << "========================================" << endl;
    cout << setprecision(6);
    cout << "\n--- Data ---" << endl;
    cout << "Original Points: " << originalCount << endl;
    cout << "Simplified Points: " << simplifiedCount << endl;
    cout << "Compression Ratio: " << compressionRatio * 100 << "%" << endl;
    cout << "\n--- Compression Performance ---" << endl;
    cout << "Total Time: " << totalTimeUs << " us" << endl;
    cout << "Average Time per Point: " << avgTimePerPointUs << " us/point" << endl;
    cout << "\n--- Decompression Performance ---" << endl;
    cout << "Total Time: " << totalDecompressTimeUs << " us" << endl;
    cout << "Average Time per Point: " << avgDecompressTimePerPointUs << " us/point" << endl;
    cout << "\n--- Accuracy ---" << endl;
    cout << "SED Threshold: " << sedMeters << " m" << endl;
    cout << "Average Error: " << globalAvgError << " m" << endl;
    cout << "Maximum Error: " << globalMaxError << " m" << endl;
    cout << "\n--- Trajectory Segments ---" << endl;
    cout << "Number of Segments: " << trajectorySegments.size() << endl;
    cout << "========================================\n" << endl;

    return 0;
}

