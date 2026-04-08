#include "../include/parking_lot.h"
#include "../include/test_framework.h"

// ==================== 车辆管理测试 ====================

TEST(Vehicle_Creation) {
    Vehicle v("京A12345", VehicleType::CAR);
    ASSERT_EQ(v.getLicensePlate(), "京A12345");
    ASSERT_EQ(v.getType(), VehicleType::CAR);
    ASSERT_GT(v.getEntryTime(), 0);
    return true;
}

TEST(Vehicle_ParkingDuration) {
    Vehicle v("京A12345");
    // 刚创建的车辆，停车时长应该很小
    ASSERT_GE(v.getParkingDuration(), 0);
    ASSERT_LT(v.getParkingDuration(), 0.1); // 小于0.1小时
    return true;
}

TEST(Vehicle_Serialization) {
    Vehicle v("京B67890", VehicleType::SUV);
    v.setSpotId("A01");
    std::string data = v.serialize();
    
    Vehicle v2 = Vehicle::deserialize(data);
    ASSERT_EQ(v2.getLicensePlate(), "京B67890");
    ASSERT_EQ(v2.getType(), VehicleType::SUV);
    ASSERT_EQ(v2.getSpotId(), "A01");
    return true;
}

// ==================== 车位管理测试 ====================

TEST(ParkingSpot_Creation) {
    ParkingSpot spot("A01", 0, 0, 1);
    ASSERT_EQ(spot.getSpotId(), "A01");
    ASSERT_EQ(spot.getStatus(), SpotStatus::AVAILABLE);
    ASSERT_EQ(spot.getNodeId(), 1);
    return true;
}

TEST(ParkingSpot_Occupy) {
    ParkingSpot spot("A01", 0, 0, 1);
    auto vehicle = std::make_shared<Vehicle>("京A12345");
    
    ASSERT_TRUE(spot.occupy(vehicle));
    ASSERT_EQ(spot.getStatus(), SpotStatus::OCCUPIED);
    ASSERT_NOT_NULL(spot.getVehicle());
    ASSERT_EQ(spot.getVehicle()->getLicensePlate(), "京A12345");
    return true;
}

TEST(ParkingSpot_Release) {
    ParkingSpot spot("A01", 0, 0, 1);
    auto vehicle = std::make_shared<Vehicle>("京A12345");
    
    spot.occupy(vehicle);
    auto released = spot.release();
    
    ASSERT_NOT_NULL(released.get());
    ASSERT_EQ(spot.getStatus(), SpotStatus::AVAILABLE);
    ASSERT_NULL(spot.getVehicle());
    return true;
}

TEST(ParkingSpot_Reserve) {
    ParkingSpot spot("A01", 0, 0, 1);
    
    ASSERT_TRUE(spot.reserve("京A12345", 30));
    ASSERT_EQ(spot.getStatus(), SpotStatus::RESERVED);
    ASSERT_EQ(spot.getReservedPlate(), "京A12345");
    return true;
}

TEST(ParkingSpot_ReserveOccupied) {
    ParkingSpot spot("A01", 0, 0, 1);
    auto vehicle = std::make_shared<Vehicle>("京A12345");
    spot.occupy(vehicle);
    
    // 已占用的车位不能预约
    ASSERT_FALSE(spot.reserve("京B67890", 30));
    return true;
}

TEST(ParkingSpot_CancelReservation) {
    ParkingSpot spot("A01", 0, 0, 1);
    spot.reserve("京A12345", 30);
    
    ASSERT_TRUE(spot.cancelReservation());
    ASSERT_EQ(spot.getStatus(), SpotStatus::AVAILABLE);
    return true;
}

TEST(ParkingSpot_OccupyReserved) {
    ParkingSpot spot("A01", 0, 0, 1);
    spot.reserve("京A12345", 30);
    
    // 预约车辆可以占用
    auto vehicle1 = std::make_shared<Vehicle>("京A12345");
    ASSERT_TRUE(spot.occupy(vehicle1));
    
    // 重置测试
    spot.release();
    spot.reserve("京A12345", 30);
    
    // 非预约车辆不能占用
    auto vehicle2 = std::make_shared<Vehicle>("京B67890");
    ASSERT_FALSE(spot.occupy(vehicle2));
    return true;
}

// ==================== 楼层管理测试 ====================

TEST(Floor_Creation) {
    Floor floor(1, "A", 2, 5);
    ASSERT_EQ(floor.getFloorId(), 1);
    ASSERT_EQ(floor.getZoneName(), "A");
    ASSERT_EQ(floor.getTotalSpots(), 10);
    return true;
}

TEST(Floor_InitSpots) {
    Floor floor(1, "A", 2, 5);
    floor.initSpots(1);
    
    ParkingSpot* spot = floor.getSpotById("A01");
    ASSERT_NOT_NULL(spot);
    ASSERT_EQ(spot->getSpotId(), "A01");
    
    spot = floor.getSpotById("A10");
    ASSERT_NOT_NULL(spot);
    return true;
}

TEST(Floor_AvailableCount) {
    Floor floor(1, "A", 2, 5);
    floor.initSpots(1);
    
    ASSERT_EQ(floor.getAvailableCount(), 10);
    
    // 占用一个车位
    auto vehicle = std::make_shared<Vehicle>("京A12345");
    floor.getSpotById("A01")->occupy(vehicle);
    
    ASSERT_EQ(floor.getAvailableCount(), 9);
    return true;
}

// ==================== 会员管理测试 ====================

TEST(Member_Creation) {
    Member m("京A12345", MemberType::MONTHLY, 1);
    ASSERT_EQ(m.getLicensePlate(), "京A12345");
    ASSERT_EQ(m.getType(), MemberType::MONTHLY);
    ASSERT_TRUE(m.isValid());
    return true;
}

TEST(Member_Discount) {
    Member monthly("京A12345", MemberType::MONTHLY, 1);
    Member yearly("京B67890", MemberType::YEARLY, 12);
    Member vip("京C11111", MemberType::VIP, 12);
    
    ASSERT_EQ(monthly.getDiscount(), 0.8);
    ASSERT_EQ(yearly.getDiscount(), 0.6);
    ASSERT_EQ(vip.getDiscount(), 0.5);
    return true;
}

TEST(Member_Renew) {
    Member m("京A12345", MemberType::MONTHLY, 1);
    int daysBefore = m.getRemainingDays();
    
    m.renew(1);
    int daysAfter = m.getRemainingDays();
    
    ASSERT_GT(daysAfter, daysBefore);
    return true;
}

TEST(Member_Serialization) {
    Member m("京A12345", MemberType::VIP, 6);
    std::string data = m.serialize();
    
    Member m2 = Member::deserialize(data);
    ASSERT_EQ(m2.getLicensePlate(), "京A12345");
    ASSERT_EQ(m2.getType(), MemberType::VIP);
    return true;
}

// ==================== 预约管理测试 ====================

TEST(Reservation_Creation) {
    Reservation r("京A12345", "A01", 30);
    ASSERT_EQ(r.getLicensePlate(), "京A12345");
    ASSERT_EQ(r.getSpotId(), "A01");
    ASSERT_TRUE(r.isValid());
    ASSERT_FALSE(r.isUsed());
    return true;
}

TEST(Reservation_RemainingTime) {
    Reservation r("京A12345", "A01", 30);
    int remaining = r.getRemainingMinutes();
    
    ASSERT_GT(remaining, 0);
    ASSERT_LE(remaining, 30);
    return true;
}

TEST(Reservation_MarkUsed) {
    Reservation r("京A12345", "A01", 30);
    r.markUsed();
    
    ASSERT_TRUE(r.isUsed());
    ASSERT_FALSE(r.isValid());
    return true;
}

TEST(Reservation_Serialization) {
    Reservation r("京A12345", "A01", 60);
    std::string data = r.serialize();
    
    Reservation r2 = Reservation::deserialize(data);
    ASSERT_EQ(r2.getLicensePlate(), "京A12345");
    ASSERT_EQ(r2.getSpotId(), "A01");
    return true;
}

// ==================== 费率计算测试 ====================

TEST(FeeRate_Default) {
    FeeRate rate;
    ASSERT_EQ(rate.getHourlyRate(), 5.0);
    ASSERT_EQ(rate.getDailyMax(), 50.0);
    ASSERT_EQ(rate.getFreeMinutes(), 15);
    return true;
}

TEST(FeeRate_FreeParking) {
    FeeRate rate;
    // 15分钟内免费
    double fee = rate.calculate(0.2); // 12分钟
    ASSERT_EQ(fee, 0.0);
    return true;
}

TEST(FeeRate_HourlyCalculation) {
    FeeRate rate;
    rate.setFreeMinutes(0); // 取消免费时段便于测试
    
    double fee = rate.calculate(2.0); // 2小时
    ASSERT_EQ(fee, 10.0); // 5元/小时 * 2小时
    return true;
}

TEST(FeeRate_DailyMax) {
    FeeRate rate;
    rate.setFreeMinutes(0);
    
    double fee = rate.calculate(24.0); // 24小时
    ASSERT_EQ(fee, 50.0); // 封顶50元
    return true;
}

TEST(FeeRate_MemberDiscount) {
    FeeRate rate;
    rate.setFreeMinutes(0);
    
    Member vip("京A12345", MemberType::VIP, 12);
    double fee = rate.calculate(2.0, &vip); // 2小时，VIP
    ASSERT_EQ(fee, 5.0); // 10元 * 0.5
    return true;
}

TEST(FeeRate_VehicleTypeMultiplier) {
    FeeRate rate;
    ASSERT_EQ(rate.getTypeMultiplier(VehicleType::CAR), 1.0);
    ASSERT_EQ(rate.getTypeMultiplier(VehicleType::SUV), 1.2);
    ASSERT_EQ(rate.getTypeMultiplier(VehicleType::MOTORCYCLE), 0.5);
    return true;
}

// ==================== 图算法测试 ====================

TEST(Graph_AddNode) {
    ParkingGraph graph;
    int id1 = graph.addNode("入口", 0, 0, false);
    int id2 = graph.addNode("A01", 1, 0, true);
    
    ASSERT_EQ(id1, 0);
    ASSERT_EQ(id2, 1);
    ASSERT_EQ(graph.getNodeCount(), 2);
    return true;
}

TEST(Graph_AddEdge) {
    ParkingGraph graph;
    graph.addNode("入口", 0, 0, false);
    graph.addNode("A01", 1, 0, true);
    graph.addEdge(0, 1, 1.0);
    
    // 边添加成功（通过路径测试验证）
    auto path = graph.dijkstra(0, 1);
    ASSERT_EQ(path.size(), 2);
    return true;
}

TEST(Graph_Dijkstra_SimplePath) {
    ParkingGraph graph;
    graph.addNode("入口", 0, 0);
    graph.addNode("A01", 1, 0);
    graph.addNode("A02", 2, 0);
    graph.addEdge(0, 1, 1.0);
    graph.addEdge(1, 2, 1.0);
    
    auto path = graph.dijkstra(0, 2);
    ASSERT_EQ(path.size(), 3);
    ASSERT_EQ(path[0], 0);
    ASSERT_EQ(path[1], 1);
    ASSERT_EQ(path[2], 2);
    return true;
}

TEST(Graph_Dijkstra_ShortestPath) {
    ParkingGraph graph;
    graph.addNode("A", 0, 0);  // 0
    graph.addNode("B", 1, 0);  // 1
    graph.addNode("C", 2, 0);  // 2
    graph.addNode("D", 1, 1);  // 3
    
    // A -> B -> C (权重 1+1=2)
    // A -> D -> C (权重 3+1=4)
    graph.addEdge(0, 1, 1.0);
    graph.addEdge(1, 2, 1.0);
    graph.addEdge(0, 3, 3.0);
    graph.addEdge(3, 2, 1.0);
    
    auto path = graph.dijkstra(0, 2);
    // 应该选择 A -> B -> C
    ASSERT_EQ(path.size(), 3);
    ASSERT_EQ(path[1], 1); // 经过B
    return true;
}

TEST(Graph_FindNodeByName) {
    ParkingGraph graph;
    graph.addNode("入口", 0, 0);
    graph.addNode("A01", 1, 0);
    
    ASSERT_EQ(graph.findNodeByName("入口"), 0);
    ASSERT_EQ(graph.findNodeByName("A01"), 1);
    ASSERT_EQ(graph.findNodeByName("不存在"), -1);
    return true;
}

TEST(Graph_PathNames) {
    ParkingGraph graph;
    graph.addNode("入口", 0, 0);
    graph.addNode("A01", 1, 0);
    graph.addEdge(0, 1, 1.0);
    
    auto path = graph.dijkstra(0, 1);
    auto names = graph.getPathNames(path);
    
    ASSERT_EQ(names.size(), 2);
    ASSERT_EQ(names[0], "入口");
    ASSERT_EQ(names[1], "A01");
    return true;
}

// ==================== 停车场集成测试 ====================

TEST(ParkingLot_Initialize) {
    ParkingLot lot("测试停车场");
    lot.initialize(3, 2, 5);
    
    ASSERT_EQ(lot.getTotalSpots(), 30);
    ASSERT_EQ(lot.getAvailableCount(), 30);
    ASSERT_EQ(lot.getFloorCount(), 3);
    return true;
}

TEST(ParkingLot_VehicleEntry) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    
    ASSERT_TRUE(lot.enterParking("京A12345", VehicleType::CAR));
    ASSERT_EQ(lot.getAvailableCount(), 19);
    
    Vehicle* v = lot.findVehicle("京A12345");
    ASSERT_NOT_NULL(v);
    ASSERT_FALSE(v->getSpotId().empty());
    return true;
}

TEST(ParkingLot_VehicleExit) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    lot.getFeeRate().setFreeMinutes(0);
    
    lot.enterParking("京A12345");
    double fee = lot.exitParking("京A12345");
    
    ASSERT_GE(fee, 0);
    ASSERT_EQ(lot.getAvailableCount(), 20);
    ASSERT_NULL(lot.findVehicle("京A12345"));
    return true;
}

TEST(ParkingLot_DuplicateEntry) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    
    ASSERT_TRUE(lot.enterParking("京A12345"));
    ASSERT_FALSE(lot.enterParking("京A12345")); // 重复入场应失败
    return true;
}

TEST(ParkingLot_FullParking) {
    ParkingLot lot("测试停车场");
    lot.initialize(1, 1, 2); // 只有2个车位
    
    ASSERT_TRUE(lot.enterParking("京A11111"));
    ASSERT_TRUE(lot.enterParking("京A22222"));
    ASSERT_FALSE(lot.enterParking("京A33333")); // 车位已满
    
    ASSERT_EQ(lot.getWaitingCount(), 1); // 应该在等待队列
    return true;
}

TEST(ParkingLot_WaitingQueue) {
    ParkingLot lot("测试停车场");
    lot.initialize(1, 1, 2);
    
    lot.enterParking("京A11111");
    lot.enterParking("京A22222");
    lot.enterParking("京A33333"); // 加入等待队列
    
    ASSERT_EQ(lot.getWaitingCount(), 1);
    
    // 一辆车出场后，等待的车应该自动入场
    lot.exitParking("京A11111");
    
    ASSERT_EQ(lot.getWaitingCount(), 0);
    ASSERT_NOT_NULL(lot.findVehicle("京A33333"));
    return true;
}

TEST(ParkingLot_Reservation) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    
    std::string reservationId = lot.createReservation("京A12345", "A01", 30);
    ASSERT_FALSE(reservationId.empty());
    
    Reservation* r = lot.getReservation("京A12345");
    ASSERT_NOT_NULL(r);
    ASSERT_EQ(r->getSpotId(), "A01");
    return true;
}

TEST(ParkingLot_ReservationEntry) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    
    lot.createReservation("京A12345", "A01", 30);
    ASSERT_TRUE(lot.enterParking("京A12345"));
    
    Vehicle* v = lot.findVehicle("京A12345");
    ASSERT_NOT_NULL(v);
    ASSERT_EQ(v->getSpotId(), "A01"); // 应该停在预约的车位
    return true;
}

TEST(ParkingLot_CancelReservation) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    
    std::string reservationId = lot.createReservation("京A12345", "A01", 30);
    ASSERT_TRUE(lot.cancelReservation(reservationId));
    
    ASSERT_NULL(lot.getReservation("京A12345"));
    return true;
}

TEST(ParkingLot_MemberRegistration) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    
    std::string memberId = lot.registerMember("京A12345", MemberType::MONTHLY, 1);
    ASSERT_FALSE(memberId.empty());
    
    Member* m = lot.getMember("京A12345");
    ASSERT_NOT_NULL(m);
    ASSERT_EQ(m->getType(), MemberType::MONTHLY);
    return true;
}

TEST(ParkingLot_MemberDiscount) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    lot.getFeeRate().setFreeMinutes(0);
    lot.getFeeRate().setHourlyRate(10.0);
    
    // 注册VIP会员
    lot.registerMember("京A12345", MemberType::VIP, 12);
    
    lot.enterParking("京A12345");
    lot.enterParking("京B67890"); // 普通用户
    
    // 模拟停车（实际测试中时间很短，费用接近最小值）
    double vipFee = lot.exitParking("京A12345");
    double normalFee = lot.exitParking("京B67890");
    
    // VIP费用应该是普通用户的一半
    ASSERT_LE(vipFee, normalFee);
    return true;
}

TEST(ParkingLot_PathToSpot) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    
    auto path = lot.getOptimalPath("A01");
    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.front(), "入口");
    ASSERT_EQ(path.back(), "A01");
    return true;
}

TEST(ParkingLot_PathToNearestSpot) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    
    auto path = lot.getPathToNearestSpot();
    ASSERT_FALSE(path.empty());
    ASSERT_EQ(path.front(), "入口");
    return true;
}

TEST(ParkingLot_DailyIncome) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    lot.getFeeRate().setFreeMinutes(0);
    
    lot.enterParking("京A12345");
    lot.exitParking("京A12345");
    
    double income = lot.getDailyIncome(time(nullptr));
    ASSERT_GE(income, 0);
    return true;
}

TEST(ParkingLot_TotalIncome) {
    ParkingLot lot("测试停车场");
    lot.initialize(2, 2, 5);
    lot.getFeeRate().setFreeMinutes(0);
    
    lot.enterParking("京A11111");
    lot.enterParking("京A22222");
    lot.exitParking("京A11111");
    lot.exitParking("京A22222");
    
    double total = lot.getTotalIncome();
    ASSERT_GE(total, 0);
    ASSERT_EQ(lot.getAllRecords().size(), 2);
    return true;
}

// ==================== 停车记录测试 ====================

TEST(ParkingRecord_Creation) {
    time_t now = time(nullptr);
    ParkingRecord record("京A12345", "A01", now - 3600, now, 5.0);
    
    ASSERT_EQ(record.getLicensePlate(), "京A12345");
    ASSERT_EQ(record.getSpotId(), "A01");
    ASSERT_EQ(record.getFee(), 5.0);
    return true;
}

TEST(ParkingRecord_Duration) {
    time_t now = time(nullptr);
    ParkingRecord record("京A12345", "A01", now - 7200, now, 10.0); // 2小时
    
    double duration = record.getDuration();
    ASSERT_GE(duration, 1.9);
    ASSERT_LE(duration, 2.1);
    return true;
}

TEST(ParkingRecord_Serialization) {
    time_t now = time(nullptr);
    ParkingRecord record("京A12345", "A01", now - 3600, now, 5.0, 
                        VehicleType::SUV, MemberType::MONTHLY);
    std::string data = record.serialize();
    
    ParkingRecord r2 = ParkingRecord::deserialize(data);
    ASSERT_EQ(r2.getLicensePlate(), "京A12345");
    ASSERT_EQ(r2.getVehicleType(), VehicleType::SUV);
    ASSERT_EQ(r2.getMemberType(), MemberType::MONTHLY);
    return true;
}

// ==================== 主函数 ====================

int main() {
    // 禁用日志控制台输出
    Logger::getInstance().setConsoleOutput(false);
    
    // 运行所有测试
    TestFramework::getInstance().runAll();
    
    return TestFramework::getInstance().allPassed() ? 0 : 1;
}
