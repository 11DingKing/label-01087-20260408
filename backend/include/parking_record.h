#ifndef PARKING_RECORD_H
#define PARKING_RECORD_H

#include "common.h"

class ParkingRecord {
public:
    ParkingRecord() : entryTime_(0), exitTime_(0), fee_(0.0), 
                      vehicleType_(VehicleType::CAR), memberType_(MemberType::NORMAL) {}
    
    ParkingRecord(const std::string& plate, const std::string& spotId,
                  time_t entry, time_t exit, double fee,
                  VehicleType vType = VehicleType::CAR,
                  MemberType mType = MemberType::NORMAL)
        : licensePlate_(plate), spotId_(spotId), entryTime_(entry),
          exitTime_(exit), fee_(fee), vehicleType_(vType), memberType_(mType) {
        recordId_ = Utils::generateId("PR");
    }
    
    // Getters
    std::string getRecordId() const { return recordId_; }
    std::string getLicensePlate() const { return licensePlate_; }
    std::string getSpotId() const { return spotId_; }
    time_t getEntryTime() const { return entryTime_; }
    time_t getExitTime() const { return exitTime_; }
    double getFee() const { return fee_; }
    VehicleType getVehicleType() const { return vehicleType_; }
    MemberType getMemberType() const { return memberType_; }
    
    // 获取停车时长（小时）
    double getDuration() const {
        return difftime(exitTime_, entryTime_) / 3600.0;
    }
    
    // 检查是否在指定日期
    bool isOnDate(time_t date) const {
        struct tm* tmEntry = localtime(&entryTime_);
        int entryDay = tmEntry->tm_yday;
        int entryYear = tmEntry->tm_year;
        
        struct tm* tmDate = localtime(&date);
        int dateDay = tmDate->tm_yday;
        int dateYear = tmDate->tm_year;
        
        return entryDay == dateDay && entryYear == dateYear;
    }
    
    // 序列化
    std::string serialize() const {
        std::stringstream ss;
        ss << recordId_ << "," << licensePlate_ << "," << spotId_ << ","
           << entryTime_ << "," << exitTime_ << "," << fee_ << ","
           << static_cast<int>(vehicleType_) << "," << static_cast<int>(memberType_);
        return ss.str();
    }
    
    // 反序列化
    static ParkingRecord deserialize(const std::string& data) {
        ParkingRecord r;
        std::stringstream ss(data);
        std::string token;
        
        std::getline(ss, r.recordId_, ',');
        std::getline(ss, r.licensePlate_, ',');
        std::getline(ss, r.spotId_, ',');
        std::getline(ss, token, ',');
        r.entryTime_ = std::stol(token);
        std::getline(ss, token, ',');
        r.exitTime_ = std::stol(token);
        std::getline(ss, token, ',');
        r.fee_ = std::stod(token);
        std::getline(ss, token, ',');
        r.vehicleType_ = static_cast<VehicleType>(std::stoi(token));
        std::getline(ss, token, ',');
        r.memberType_ = static_cast<MemberType>(std::stoi(token));
        
        return r;
    }
    
private:
    std::string recordId_;      // 记录ID
    std::string licensePlate_;  // 车牌号
    std::string spotId_;        // 车位ID
    time_t entryTime_;          // 入场时间
    time_t exitTime_;           // 出场时间
    double fee_;                // 费用
    VehicleType vehicleType_;   // 车辆类型
    MemberType memberType_;     // 会员类型
};

#endif // PARKING_RECORD_H
