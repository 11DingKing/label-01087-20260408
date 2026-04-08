#ifndef PARKING_SPOT_H
#define PARKING_SPOT_H

#include "common.h"
#include "vehicle.h"

class ParkingSpot {
public:
    ParkingSpot() : status_(SpotStatus::AVAILABLE), x_(0), y_(0), nodeId_(-1) {}
    
    ParkingSpot(const std::string& id, int x, int y, int nodeId)
        : spotId_(id), status_(SpotStatus::AVAILABLE), x_(x), y_(y), nodeId_(nodeId) {}
    
    // Getters
    std::string getSpotId() const { return spotId_; }
    SpotStatus getStatus() const { return status_; }
    int getX() const { return x_; }
    int getY() const { return y_; }
    int getNodeId() const { return nodeId_; }
    Vehicle* getVehicle() const { return vehicle_.get(); }
    std::string getReservedPlate() const { return reservedPlate_; }
    time_t getReserveExpireTime() const { return reserveExpireTime_; }
    
    // 占用车位
    bool occupy(std::shared_ptr<Vehicle> vehicle) {
        if (status_ != SpotStatus::AVAILABLE && status_ != SpotStatus::RESERVED) {
            return false;
        }
        // 如果是预约车位，检查是否是预约的车辆
        if (status_ == SpotStatus::RESERVED && 
            vehicle->getLicensePlate() != reservedPlate_) {
            return false;
        }
        
        vehicle_ = vehicle;
        vehicle_->setSpotId(spotId_);
        status_ = SpotStatus::OCCUPIED;
        reservedPlate_.clear();
        LOG_INFO("车位 " + spotId_ + " 被车辆 " + vehicle->getLicensePlate() + " 占用");
        return true;
    }
    
    // 释放车位
    std::shared_ptr<Vehicle> release() {
        if (status_ != SpotStatus::OCCUPIED) {
            return nullptr;
        }
        auto v = vehicle_;
        vehicle_ = nullptr;
        status_ = SpotStatus::AVAILABLE;
        LOG_INFO("车位 " + spotId_ + " 已释放");
        return v;
    }
    
    // 预约车位
    bool reserve(const std::string& plate, int minutes = 30) {
        if (status_ != SpotStatus::AVAILABLE) {
            return false;
        }
        status_ = SpotStatus::RESERVED;
        reservedPlate_ = plate;
        reserveExpireTime_ = time(nullptr) + minutes * 60;
        LOG_INFO("车位 " + spotId_ + " 被车辆 " + plate + " 预约");
        return true;
    }
    
    // 取消预约
    bool cancelReservation() {
        if (status_ != SpotStatus::RESERVED) {
            return false;
        }
        status_ = SpotStatus::AVAILABLE;
        reservedPlate_.clear();
        reserveExpireTime_ = 0;
        LOG_INFO("车位 " + spotId_ + " 预约已取消");
        return true;
    }
    
    // 检查预约是否过期
    bool isReservationExpired() const {
        if (status_ != SpotStatus::RESERVED) return false;
        return time(nullptr) > reserveExpireTime_;
    }
    
    // 设置维护状态
    void setMaintenance(bool enabled) {
        if (enabled && status_ == SpotStatus::AVAILABLE) {
            status_ = SpotStatus::MAINTENANCE;
        } else if (!enabled && status_ == SpotStatus::MAINTENANCE) {
            status_ = SpotStatus::AVAILABLE;
        }
    }
    
    // 获取显示字符
    std::string getDisplayChar() const {
        switch (status_) {
            case SpotStatus::AVAILABLE: return "□";
            case SpotStatus::OCCUPIED: return "■";
            case SpotStatus::RESERVED: return "◆";
            case SpotStatus::MAINTENANCE: return "✕";
            default: return "?";
        }
    }
    
    // 获取带颜色的显示字符
    std::string getColoredDisplayChar() const {
        switch (status_) {
            case SpotStatus::AVAILABLE: 
                return Color::GREEN + "□" + Color::RESET;
            case SpotStatus::OCCUPIED: 
                return Color::RED + "■" + Color::RESET;
            case SpotStatus::RESERVED: 
                return Color::YELLOW + "◆" + Color::RESET;
            case SpotStatus::MAINTENANCE: 
                return Color::MAGENTA + "✕" + Color::RESET;
            default: 
                return "?";
        }
    }
    
    // 序列化
    std::string serialize() const {
        std::stringstream ss;
        ss << spotId_ << "," << static_cast<int>(status_) << ","
           << x_ << "," << y_ << "," << nodeId_ << ","
           << reservedPlate_ << "," << reserveExpireTime_;
        if (vehicle_) {
            ss << "," << vehicle_->serialize();
        }
        return ss.str();
    }
    
private:
    std::string spotId_;                // 车位编号
    SpotStatus status_;                 // 车位状态
    int x_, y_;                         // 坐标位置
    int nodeId_;                        // 图节点ID
    std::shared_ptr<Vehicle> vehicle_;  // 停放的车辆
    std::string reservedPlate_;         // 预约车牌
    time_t reserveExpireTime_;          // 预约过期时间
};

#endif // PARKING_SPOT_H
