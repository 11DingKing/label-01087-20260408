#ifndef MEMBER_H
#define MEMBER_H

#include "common.h"

class Member {
public:
    Member() : type_(MemberType::NORMAL), expireDate_(0), createDate_(0) {}
    
    Member(const std::string& plate, MemberType type, int months = 1)
        : licensePlate_(plate), type_(type), createDate_(time(nullptr)) {
        memberId_ = Utils::generateId("M");
        expireDate_ = createDate_ + months * 30 * 24 * 3600; // 简化计算
    }
    
    // Getters
    std::string getMemberId() const { return memberId_; }
    std::string getLicensePlate() const { return licensePlate_; }
    MemberType getType() const { return type_; }
    time_t getExpireDate() const { return expireDate_; }
    time_t getCreateDate() const { return createDate_; }
    
    // 检查会员是否有效
    bool isValid() const {
        return time(nullptr) < expireDate_;
    }
    
    // 获取折扣率
    double getDiscount() const {
        if (!isValid()) return 1.0;
        switch (type_) {
            case MemberType::MONTHLY: return 0.8;   // 8折
            case MemberType::YEARLY: return 0.6;    // 6折
            case MemberType::VIP: return 0.5;       // 5折
            default: return 1.0;
        }
    }
    
    // 续费
    void renew(int months) {
        time_t now = time(nullptr);
        if (expireDate_ < now) {
            expireDate_ = now + months * 30 * 24 * 3600;
        } else {
            expireDate_ += months * 30 * 24 * 3600;
        }
        LOG_INFO("会员 " + memberId_ + " 续费 " + std::to_string(months) + " 个月");
    }
    
    // 升级会员类型
    void upgrade(MemberType newType) {
        if (static_cast<int>(newType) > static_cast<int>(type_)) {
            type_ = newType;
            LOG_INFO("会员 " + memberId_ + " 升级为 " + Utils::memberTypeToStr(type_));
        }
    }
    
    // 获取剩余天数
    int getRemainingDays() const {
        time_t now = time(nullptr);
        if (expireDate_ <= now) return 0;
        return static_cast<int>(difftime(expireDate_, now) / (24 * 3600));
    }
    
    // 序列化
    std::string serialize() const {
        std::stringstream ss;
        ss << memberId_ << "," << licensePlate_ << ","
           << static_cast<int>(type_) << "," << expireDate_ << "," << createDate_;
        return ss.str();
    }
    
    // 反序列化
    static Member deserialize(const std::string& data) {
        Member m;
        std::stringstream ss(data);
        std::string token;
        
        std::getline(ss, m.memberId_, ',');
        std::getline(ss, m.licensePlate_, ',');
        std::getline(ss, token, ',');
        m.type_ = static_cast<MemberType>(std::stoi(token));
        std::getline(ss, token, ',');
        m.expireDate_ = std::stol(token);
        std::getline(ss, token, ',');
        m.createDate_ = std::stol(token);
        
        return m;
    }
    
private:
    std::string memberId_;      // 会员ID
    std::string licensePlate_;  // 车牌号
    MemberType type_;           // 会员类型
    time_t expireDate_;         // 到期时间
    time_t createDate_;         // 创建时间
};

#endif // MEMBER_H
