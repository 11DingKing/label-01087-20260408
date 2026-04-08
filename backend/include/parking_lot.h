#ifndef PARKING_LOT_H
#define PARKING_LOT_H

#include "common.h"
#include "floor.h"
#include "graph.h"
#include "vehicle.h"
#include "member.h"
#include "reservation.h"
#include "fee_rate.h"
#include "parking_record.h"

class ParkingLot {
public:
    ParkingLot(const std::string& name = "智能停车场");
    ~ParkingLot() = default;
    
    // 初始化停车场
    void initialize(int numFloors, int rows, int cols);
    
    // 车辆管理
    bool enterParking(const std::string& plate, VehicleType type = VehicleType::CAR);
    double exitParking(const std::string& plate);
    Vehicle* findVehicle(const std::string& plate);
    
    // 车位管理
    std::vector<ParkingSpot*> getAvailableSpots();
    int getTotalSpots() const;
    int getAvailableCount() const;
    int getOccupiedCount() const;
    ParkingSpot* getSpotById(const std::string& spotId);
    
    // 路径规划
    std::vector<std::string> getOptimalPath(const std::string& spotId);
    std::vector<std::string> getPathToNearestSpot();
    
    // 预约管理
    std::string createReservation(const std::string& plate, const std::string& spotId, int minutes = 30);
    bool cancelReservation(const std::string& reservationId);
    Reservation* getReservation(const std::string& plate);
    std::vector<Reservation>& getReservations() { return reservations_; }
    
    // 会员管理
    std::string registerMember(const std::string& plate, MemberType type, int months = 1);
    bool renewMembership(const std::string& memberId, int months);
    Member* getMember(const std::string& plate);
    Member* getMemberById(const std::string& memberId);
    std::vector<Member>& getMembers() { return members_; }
    
    // 费率管理
    FeeRate& getFeeRate() { return feeRate_; }
    void setFeeRate(double hourly, double dailyMax);
    
    // 记录管理
    std::vector<ParkingRecord> getRecords(time_t start, time_t end);
    double getDailyIncome(time_t date);
    double getTotalIncome();
    std::vector<ParkingRecord>& getAllRecords() { return records_; }
    
    // 等待队列
    void addToWaitingQueue(const std::string& plate, VehicleType type);
    bool processWaitingQueue();
    int getWaitingCount() const { return waitingQueue_.size(); }
    
    // 显示功能
    void displayAllFloors() const;
    void displayStatistics() const;
    void displayPath(const std::vector<std::string>& path) const;
    
    // 数据持久化
    void saveData();
    void loadData();
    
    // Getters
    std::string getName() const { return name_; }
    int getFloorCount() const { return floors_.size(); }
    Floor* getFloor(int index);
    ParkingGraph& getGraph() { return graph_; }
    
private:
    std::string name_;                              // 停车场名称
    std::vector<Floor> floors_;                     // 楼层列表
    ParkingGraph graph_;                            // 路径图
    int entryNodeId_;                               // 入口节点ID
    
    std::map<std::string, std::shared_ptr<Vehicle>> parkedVehicles_;  // 在场车辆
    std::queue<std::pair<std::string, VehicleType>> waitingQueue_;    // 等待队列
    
    std::vector<Member> members_;                   // 会员列表
    std::vector<Reservation> reservations_;         // 预约列表
    std::vector<ParkingRecord> records_;            // 停车记录
    FeeRate feeRate_;                               // 费率配置
    
    // 私有方法
    void buildGraph();
    ParkingSpot* findAvailableSpot();
    void cleanExpiredReservations();
};

#endif // PARKING_LOT_H
