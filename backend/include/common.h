#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <memory>
#include <functional>
#include <filesystem>

// ANSI颜色代码
namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string RED     = "\033[1;31m";
    const std::string GREEN   = "\033[1;32m";
    const std::string YELLOW  = "\033[1;33m";
    const std::string BLUE    = "\033[1;34m";
    const std::string MAGENTA = "\033[1;35m";
    const std::string CYAN    = "\033[1;36m";
    const std::string WHITE   = "\033[1;37m";
}

// 枚举定义
enum class SpotStatus {
    AVAILABLE,      // 空闲
    OCCUPIED,       // 占用
    RESERVED,       // 已预约
    MAINTENANCE     // 维护中
};

enum class VehicleType {
    CAR,            // 小轿车
    SUV,            // SUV
    MOTORCYCLE      // 摩托车
};

enum class MemberType {
    NORMAL,         // 普通用户
    MONTHLY,        // 月卡用户
    YEARLY,         // 年卡用户
    VIP             // VIP用户
};

// 工具函数
namespace Utils {
    inline std::string getCurrentTimeStr() {
        time_t now = time(nullptr);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }
    
    inline std::string timeToStr(time_t t) {
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
        return std::string(buf);
    }
    
    inline time_t strToTime(const std::string& str) {
        struct tm tm = {};
        sscanf(str.c_str(), "%d-%d-%d %d:%d:%d",
               &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
               &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        return mktime(&tm);
    }
    
    inline std::string generateId(const std::string& prefix) {
        static int counter = 0;
        std::stringstream ss;
        ss << prefix << std::setfill('0') << std::setw(6) << (++counter);
        return ss.str();
    }
    
    inline std::string spotStatusToStr(SpotStatus status) {
        switch (status) {
            case SpotStatus::AVAILABLE: return "空闲";
            case SpotStatus::OCCUPIED: return "占用";
            case SpotStatus::RESERVED: return "预约";
            case SpotStatus::MAINTENANCE: return "维护";
            default: return "未知";
        }
    }
    
    inline std::string memberTypeToStr(MemberType type) {
        switch (type) {
            case MemberType::NORMAL: return "普通";
            case MemberType::MONTHLY: return "月卡";
            case MemberType::YEARLY: return "年卡";
            case MemberType::VIP: return "VIP";
            default: return "未知";
        }
    }
    
    inline std::string vehicleTypeToStr(VehicleType type) {
        switch (type) {
            case VehicleType::CAR: return "轿车";
            case VehicleType::SUV: return "SUV";
            case VehicleType::MOTORCYCLE: return "摩托";
            default: return "未知";
        }
    }
    
    inline void clearScreen() {
        // 使用ANSI转义序列清屏，跨平台兼容
        std::cout << "\033[2J\033[1;1H" << std::flush;
    }
    
    inline void pause() {
        std::cout << "\n按回车键继续...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }
}

// 增强日志系统
class Logger {
public:
    enum Level { DEBUG, INFO, WARN, ERROR };
    
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void log(Level level, const std::string& message, 
             const std::string& file = "", int line = 0) {
        if (level < minLevel_) return;
        
        std::string levelStr;
        std::string color;
        switch (level) {
            case DEBUG: levelStr = "DEBUG"; color = Color::WHITE; break;
            case INFO:  levelStr = "INFO";  color = Color::GREEN; break;
            case WARN:  levelStr = "WARN";  color = Color::YELLOW; break;
            case ERROR: levelStr = "ERROR"; color = Color::RED; break;
        }
        
        std::stringstream ss;
        ss << "[" << Utils::getCurrentTimeStr() << "] "
           << "[" << levelStr << "] ";
        
        if (!file.empty()) {
            // 只保留文件名
            size_t pos = file.find_last_of("/\\");
            std::string filename = (pos != std::string::npos) ? file.substr(pos + 1) : file;
            ss << "[" << filename << ":" << line << "] ";
        }
        
        ss << message;
        std::string logLine = ss.str();
        
        // 输出到控制台
        if (consoleOutput_) {
            std::cout << color << logLine << Color::RESET << std::endl;
        }
        
        // 输出到文件
        if (logFile_.is_open()) {
            logFile_ << logLine << std::endl;
            logFile_.flush();
        }
        
        // 统计
        ++logCounts_[level];
    }
    
    void setConsoleOutput(bool enabled) { consoleOutput_ = enabled; }
    void setMinLevel(Level level) { minLevel_ = level; }
    
    void setLogFile(const std::string& filename) {
        if (logFile_.is_open()) logFile_.close();
        // 使用 std::filesystem 确保目录存在
        std::error_code ec;
        std::filesystem::create_directories("data", ec);
        logFile_.open(filename, std::ios::app);
        if (logFile_.is_open()) {
            logFile_ << "\n========== 日志会话开始 " << Utils::getCurrentTimeStr() << " ==========\n";
        }
    }
    
    // 获取日志统计
    int getLogCount(Level level) const {
        auto it = logCounts_.find(level);
        return it != logCounts_.end() ? it->second : 0;
    }
    
    void resetCounts() { logCounts_.clear(); }
    
    // 打印日志统计
    void printStats() const {
        std::cout << "\n日志统计:\n";
        std::cout << "  DEBUG: " << getLogCount(DEBUG) << "\n";
        std::cout << "  INFO:  " << getLogCount(INFO) << "\n";
        std::cout << "  WARN:  " << getLogCount(WARN) << "\n";
        std::cout << "  ERROR: " << getLogCount(ERROR) << "\n";
    }
    
private:
    Logger() : consoleOutput_(false), minLevel_(INFO) {
        setLogFile("data/system.log");
    }
    ~Logger() { 
        if (logFile_.is_open()) {
            logFile_ << "========== 日志会话结束 " << Utils::getCurrentTimeStr() << " ==========\n";
            logFile_.close(); 
        }
    }
    
    bool consoleOutput_;
    Level minLevel_;
    std::ofstream logFile_;
    std::map<Level, int> logCounts_;
};

#define LOG_DEBUG(msg) Logger::getInstance().log(Logger::DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  Logger::getInstance().log(Logger::INFO, msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  Logger::getInstance().log(Logger::WARN, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::getInstance().log(Logger::ERROR, msg, __FILE__, __LINE__)

#endif // COMMON_H
