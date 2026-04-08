#ifndef RESERVATION_H
#define RESERVATION_H

#include "common.h"

class Reservation {
public:
    Reservation() : reserveTime_(0), expireTime_(0), used_(false) {}
    
    Reservation(const std::string& plate, const std::string& spotId, int minutes = 30)
        : licensePlate_(plate), spotId_(spotId), used_(false) {
        reservationId_ = Utils::generateId("R");
        reserveTime_ = time(nullptr);
        expireTime_ = reserveTime_ + minutes * 60;
    }
    
    // Getters
    std::string getReservationId() const { return reservationId_; }
    std::string getLicensePlate() const { return licensePlate_; }
    std::string getSpotId() const { return spotId_; }
    time_t getReserveTime() const { return reserveTime_; }
    time_t getExpireTime() const { return expireTime_; }
    bool isUsed() const { return used_; }
    
    // 检查是否过期
    bool isExpired() const {
        return time(nullptr) > expireTime_;
    }
    
    // 检查是否有效（未过期且未使用）
    bool isValid() const {
        return !isExpired() && !used_;
    }
    
    // 标记为已使用
    void markUsed() {
        used_ = true;
        LOG_INFO("预约 " + reservationId_ + " 已使用");
    }
    
    // 获取剩余时间（分钟）
    int getRemainingMinutes() const {
        time_t now = time(nullptr);
        if (expireTime_ <= now) return 0;
        return static_cast<int>(difftime(expireTime_, now) / 60);
    }
    
    // 序列化
    std::string serialize() const {
        std::stringstream ss;
        ss << reservationId_ << "," << licensePlate_ << ","
           << spotId_ << "," << reserveTime_ << ","
           << expireTime_ << "," << (used_ ? 1 : 0);
        return ss.str();
    }
    
    // 反序列化
    static Reservation deserialize(const std::string& data) {
        Reservation r;
        std::stringstream ss(data);
        std::string token;
        
        std::getline(ss, r.reservationId_, ',');
        std::getline(ss, r.licensePlate_, ',');
        std::getline(ss, r.spotId_, ',');
        std::getline(ss, token, ',');
        r.reserveTime_ = std::stol(token);
        std::getline(ss, token, ',');
        r.expireTime_ = std::stol(token);
        std::getline(ss, token, ',');
        r.used_ = (std::stoi(token) == 1);
        
        return r;
    }
    
private:
    std::string reservationId_;  // 预约ID
    std::string licensePlate_;   // 车牌号
    std::string spotId_;         // 预约车位
    time_t reserveTime_;         // 预约时间
    time_t expireTime_;          // 过期时间
    bool used_;                  // 是否已使用
};

#endif // RESERVATION_H
