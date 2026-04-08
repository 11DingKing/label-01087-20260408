#ifndef VEHICLE_H
#define VEHICLE_H

#include "common.h"

class Vehicle {
public:
    Vehicle() : type_(VehicleType::CAR), entryTime_(0) {}
    
    Vehicle(const std::string& plate, VehicleType type = VehicleType::CAR)
        : licensePlate_(plate), type_(type), entryTime_(time(nullptr)) {}
    
    // Getters
    std::string getLicensePlate() const { return licensePlate_; }
    time_t getEntryTime() const { return entryTime_; }
    std::string getSpotId() const { return spotId_; }
    VehicleType getType() const { return type_; }
    
    // Setters
    void setSpotId(const std::string& spotId) { spotId_ = spotId; }
    void setEntryTime(time_t t) { entryTime_ = t; }
    
    // 计算停车时长（小时）
    double getParkingDuration() const {
        time_t now = time(nullptr);
        double hours = difftime(now, entryTime_) / 3600.0;
        return hours > 0 ? hours : 0.01; // 最少按0.01小时计算
    }
    
    // 序列化
    std::string serialize() const {
        std::stringstream ss;
        ss << licensePlate_ << "," << static_cast<int>(type_) << ","
           << entryTime_ << "," << spotId_;
        return ss.str();
    }
    
    // 反序列化
    static Vehicle deserialize(const std::string& data) {
        Vehicle v;
        std::stringstream ss(data);
        std::string token;
        
        std::getline(ss, v.licensePlate_, ',');
        std::getline(ss, token, ',');
        v.type_ = static_cast<VehicleType>(std::stoi(token));
        std::getline(ss, token, ',');
        v.entryTime_ = std::stol(token);
        std::getline(ss, v.spotId_, ',');
        
        return v;
    }
    
private:
    std::string licensePlate_;  // 车牌号
    VehicleType type_;          // 车辆类型
    time_t entryTime_;          // 入场时间
    std::string spotId_;        // 停放车位ID
};

#endif // VEHICLE_H
