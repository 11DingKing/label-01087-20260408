#ifndef FEE_RATE_H
#define FEE_RATE_H

#include "common.h"
#include "member.h"
#include <cmath>

class FeeRate {
public:
    FeeRate() : hourlyRate_(5.0), dailyMax_(50.0), freeMinutes_(15) {}
    
    // Getters
    double getHourlyRate() const { return hourlyRate_; }
    double getDailyMax() const { return dailyMax_; }
    int getFreeMinutes() const { return freeMinutes_; }
    
    // Setters
    void setHourlyRate(double rate) { 
        hourlyRate_ = rate; 
        LOG_INFO("小时费率设置为: " + std::to_string(rate) + " 元");
    }
    void setDailyMax(double max) { 
        dailyMax_ = max; 
        LOG_INFO("每日封顶设置为: " + std::to_string(max) + " 元");
    }
    void setFreeMinutes(int minutes) { 
        freeMinutes_ = minutes; 
        LOG_INFO("免费时长设置为: " + std::to_string(minutes) + " 分钟");
    }
    
    // 计算费用
    double calculate(double hours, const Member* member = nullptr) const {
        // 免费时段
        if (hours * 60 <= freeMinutes_) {
            return 0.0;
        }
        
        // 基础费用计算
        double fee = hours * hourlyRate_;
        
        // 每日封顶
        int days = static_cast<int>(hours / 24);
        double remainingHours = hours - days * 24;
        fee = days * dailyMax_ + std::min(remainingHours * hourlyRate_, dailyMax_);
        
        // 会员折扣
        if (member && member->isValid()) {
            fee *= member->getDiscount();
        }
        
        // 四舍五入到两位小数
        return std::round(fee * 100) / 100;
    }
    
    // 根据车辆类型调整费率
    double getTypeMultiplier(VehicleType type) const {
        switch (type) {
            case VehicleType::MOTORCYCLE: return 0.5;  // 摩托车半价
            case VehicleType::SUV: return 1.2;         // SUV加价20%
            default: return 1.0;
        }
    }
    
    // 序列化
    std::string serialize() const {
        std::stringstream ss;
        ss << hourlyRate_ << "," << dailyMax_ << "," << freeMinutes_;
        return ss.str();
    }
    
    // 反序列化
    static FeeRate deserialize(const std::string& data) {
        FeeRate f;
        std::stringstream ss(data);
        std::string token;
        
        std::getline(ss, token, ',');
        f.hourlyRate_ = std::stod(token);
        std::getline(ss, token, ',');
        f.dailyMax_ = std::stod(token);
        std::getline(ss, token, ',');
        f.freeMinutes_ = std::stoi(token);
        
        return f;
    }
    
private:
    double hourlyRate_;   // 每小时费率
    double dailyMax_;     // 每日封顶
    int freeMinutes_;     // 免费时长（分钟）
};

#endif // FEE_RATE_H
