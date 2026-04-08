#include "../include/parking_lot.h"
#include <filesystem>
#include <random>

ParkingLot::ParkingLot(const std::string& name) : name_(name), entryNodeId_(-1) {}

void ParkingLot::initialize(int numFloors, int rows, int cols) {
    floors_.clear();
    
    // 动态生成区域名称，支持任意数量楼层
    auto generateZoneName = [](int index) -> std::string {
        std::string name;
        do {
            name = static_cast<char>('A' + (index % 26)) + name;
            index = index / 26 - 1;
        } while (index >= 0);
        return name;
    };
    
    int nodeIdCounter = 1;  // 0 留给入口
    
    for (int i = 0; i < numFloors; ++i) {
        std::string zoneName = generateZoneName(i);
        Floor floor(i + 1, zoneName, rows, cols);
        floor.initSpots(nodeIdCounter);
        nodeIdCounter += rows * cols;
        floors_.push_back(floor);
    }
    
    // 构建路径图
    buildGraph();
    
    LOG_INFO("停车场初始化完成: " + std::to_string(numFloors) + "层, 共" + 
             std::to_string(getTotalSpots()) + "个车位");
}

void ParkingLot::buildGraph() {
    // 添加入口节点
    entryNodeId_ = graph_.addNode("入口", 0, 0, false);
    
    // 添加通道节点和车位节点
    for (auto& floor : floors_) {
        // 添加楼层入口通道
        std::string floorEntry = floor.getZoneName() + "入口";
        int floorEntryId = graph_.addNode(floorEntry, 0, floor.getFloorId(), false);
        
        // 连接到总入口或上一层
        if (floor.getFloorId() == 1) {
            graph_.addEdge(entryNodeId_, floorEntryId, 1.0);
        }
        
        // 添加车位节点并连接
        auto& spots = floor.getSpots();
        int prevRowFirstId = floorEntryId;
        
        for (int i = 0; i < floor.getRows(); ++i) {
            for (int j = 0; j < floor.getCols(); ++j) {
                auto& spot = spots[i][j];
                int spotNodeId = graph_.addNode(spot.getSpotId(), j, i, true);
                
                // 连接到同行前一个车位
                if (j > 0) {
                    graph_.addEdge(spotNodeId - 1, spotNodeId, 1.0);
                }
                
                // 第一列连接到楼层入口或上一行第一个
                if (j == 0) {
                    if (i == 0) {
                        graph_.addEdge(floorEntryId, spotNodeId, 1.0);
                    } else {
                        graph_.addEdge(prevRowFirstId, spotNodeId, 1.5);
                    }
                    prevRowFirstId = spotNodeId;
                }
            }
        }
        
        // 连接到下一层
        if (floor.getFloorId() < static_cast<int>(floors_.size())) {
            std::string nextFloorEntry = floors_[floor.getFloorId()].getZoneName() + "入口";
            int nextEntryId = graph_.addNode(nextFloorEntry, 0, floor.getFloorId() + 1, false);
            // 从当前层最后一个车位连接到下一层入口
            int lastSpotId = spots.back().back().getNodeId();
            graph_.addEdge(lastSpotId, nextEntryId, 2.0);
        }
    }
}

bool ParkingLot::enterParking(const std::string& plate, VehicleType type) {
    // 检查是否已在场
    if (parkedVehicles_.find(plate) != parkedVehicles_.end()) {
        LOG_WARN("车辆 " + plate + " 已在停车场内");
        return false;
    }
    
    // 清理过期预约
    cleanExpiredReservations();
    
    // 检查是否有预约
    Reservation* reservation = getReservation(plate);
    ParkingSpot* spot = nullptr;
    
    if (reservation && reservation->isValid()) {
        // 使用预约车位
        spot = getSpotById(reservation->getSpotId());
        reservation->markUsed();
    } else {
        // 查找空闲车位
        spot = findAvailableSpot();
    }
    
    if (!spot) {
        LOG_WARN("没有可用车位，车辆 " + plate + " 加入等待队列");
        addToWaitingQueue(plate, type);
        return false;
    }
    
    // 创建车辆并停入
    auto vehicle = std::make_shared<Vehicle>(plate, type);
    if (spot->occupy(vehicle)) {
        parkedVehicles_[plate] = vehicle;
        LOG_INFO("车辆 " + plate + " 入场，停放于 " + spot->getSpotId());
        return true;
    }
    
    return false;
}

double ParkingLot::exitParking(const std::string& plate) {
    auto it = parkedVehicles_.find(plate);
    if (it == parkedVehicles_.end()) {
        LOG_WARN("未找到车辆: " + plate);
        return -1;
    }
    
    auto vehicle = it->second;
    ParkingSpot* spot = getSpotById(vehicle->getSpotId());
    
    if (!spot) {
        LOG_ERROR("车位数据异常: " + vehicle->getSpotId());
        return -1;
    }
    
    // 计算费用
    double hours = vehicle->getParkingDuration();
    Member* member = getMember(plate);
    double fee = feeRate_.calculate(hours, member);
    fee *= feeRate_.getTypeMultiplier(vehicle->getType());
    
    // 创建停车记录
    ParkingRecord record(plate, vehicle->getSpotId(), vehicle->getEntryTime(),
                        time(nullptr), fee, vehicle->getType(),
                        member ? member->getType() : MemberType::NORMAL);
    records_.push_back(record);
    
    // 释放车位
    spot->release();
    parkedVehicles_.erase(it);
    
    LOG_INFO("车辆 " + plate + " 出场，费用: " + std::to_string(fee) + " 元");
    
    // 处理等待队列
    processWaitingQueue();
    
    return fee;
}

Vehicle* ParkingLot::findVehicle(const std::string& plate) {
    auto it = parkedVehicles_.find(plate);
    return it != parkedVehicles_.end() ? it->second.get() : nullptr;
}

std::vector<ParkingSpot*> ParkingLot::getAvailableSpots() {
    std::vector<ParkingSpot*> available;
    for (auto& floor : floors_) {
        auto floorSpots = floor.getAvailableSpots();
        available.insert(available.end(), floorSpots.begin(), floorSpots.end());
    }
    return available;
}

int ParkingLot::getTotalSpots() const {
    int total = 0;
    for (const auto& floor : floors_) {
        total += floor.getTotalSpots();
    }
    return total;
}

int ParkingLot::getAvailableCount() const {
    int count = 0;
    for (const auto& floor : floors_) {
        count += floor.getAvailableCount();
    }
    return count;
}

int ParkingLot::getOccupiedCount() const {
    return getTotalSpots() - getAvailableCount();
}

ParkingSpot* ParkingLot::getSpotById(const std::string& spotId) {
    for (auto& floor : floors_) {
        ParkingSpot* spot = floor.getSpotById(spotId);
        if (spot) return spot;
    }
    return nullptr;
}

ParkingSpot* ParkingLot::findAvailableSpot() {
    for (auto& floor : floors_) {
        auto spots = floor.getAvailableSpots();
        if (!spots.empty()) {
            return spots[0];
        }
    }
    return nullptr;
}

std::vector<std::string> ParkingLot::getOptimalPath(const std::string& spotId) {
    int spotNodeId = graph_.findNodeByName(spotId);
    if (spotNodeId < 0) {
        return {};
    }
    
    auto path = graph_.dijkstra(entryNodeId_, spotNodeId);
    return graph_.getPathNames(path);
}

std::vector<std::string> ParkingLot::getPathToNearestSpot() {
    std::vector<int> availableNodes;
    for (auto& floor : floors_) {
        auto nodes = floor.getAvailableSpotNodeIds();
        availableNodes.insert(availableNodes.end(), nodes.begin(), nodes.end());
    }
    
    if (availableNodes.empty()) {
        return {};
    }
    
    int nearestNode = graph_.findNearestAvailableSpot(entryNodeId_, availableNodes);
    if (nearestNode < 0) {
        return {};
    }
    
    auto path = graph_.dijkstra(entryNodeId_, nearestNode);
    return graph_.getPathNames(path);
}

std::string ParkingLot::createReservation(const std::string& plate, 
                                          const std::string& spotId, int minutes) {
    // 检查车位是否可预约
    ParkingSpot* spot = getSpotById(spotId);
    if (!spot || spot->getStatus() != SpotStatus::AVAILABLE) {
        return "";
    }
    
    // 检查是否已有预约
    for (const auto& r : reservations_) {
        if (r.getLicensePlate() == plate && r.isValid()) {
            return "";  // 已有有效预约
        }
    }
    
    // 创建预约
    Reservation reservation(plate, spotId, minutes);
    spot->reserve(plate, minutes);
    reservations_.push_back(reservation);
    
    LOG_INFO("创建预约: " + reservation.getReservationId() + 
             " 车牌: " + plate + " 车位: " + spotId);
    
    return reservation.getReservationId();
}

bool ParkingLot::cancelReservation(const std::string& reservationId) {
    for (auto& r : reservations_) {
        if (r.getReservationId() == reservationId && r.isValid()) {
            ParkingSpot* spot = getSpotById(r.getSpotId());
            if (spot) {
                spot->cancelReservation();
            }
            r.markUsed();  // 标记为已使用（取消）
            LOG_INFO("取消预约: " + reservationId);
            return true;
        }
    }
    return false;
}

Reservation* ParkingLot::getReservation(const std::string& plate) {
    for (auto& r : reservations_) {
        if (r.getLicensePlate() == plate && r.isValid()) {
            return &r;
        }
    }
    return nullptr;
}

std::string ParkingLot::registerMember(const std::string& plate, MemberType type, int months) {
    // 检查是否已是会员
    for (auto& m : members_) {
        if (m.getLicensePlate() == plate) {
            if (m.isValid()) {
                return "";  // 已是有效会员
            } else {
                // 续费
                m.renew(months);
                m.upgrade(type);
                return m.getMemberId();
            }
        }
    }
    
    // 创建新会员
    Member member(plate, type, months);
    members_.push_back(member);
    LOG_INFO("注册会员: " + member.getMemberId() + " 车牌: " + plate);
    return member.getMemberId();
}

bool ParkingLot::renewMembership(const std::string& memberId, int months) {
    for (auto& m : members_) {
        if (m.getMemberId() == memberId) {
            m.renew(months);
            return true;
        }
    }
    return false;
}

Member* ParkingLot::getMember(const std::string& plate) {
    for (auto& m : members_) {
        if (m.getLicensePlate() == plate && m.isValid()) {
            return &m;
        }
    }
    return nullptr;
}

Member* ParkingLot::getMemberById(const std::string& memberId) {
    for (auto& m : members_) {
        if (m.getMemberId() == memberId) {
            return &m;
        }
    }
    return nullptr;
}

void ParkingLot::setFeeRate(double hourly, double dailyMax) {
    feeRate_.setHourlyRate(hourly);
    feeRate_.setDailyMax(dailyMax);
}

std::vector<ParkingRecord> ParkingLot::getRecords(time_t start, time_t end) {
    std::vector<ParkingRecord> result;
    for (const auto& r : records_) {
        if (r.getEntryTime() >= start && r.getEntryTime() <= end) {
            result.push_back(r);
        }
    }
    return result;
}

double ParkingLot::getDailyIncome(time_t date) {
    double total = 0;
    for (const auto& r : records_) {
        if (r.isOnDate(date)) {
            total += r.getFee();
        }
    }
    return total;
}

double ParkingLot::getTotalIncome() {
    double total = 0;
    for (const auto& r : records_) {
        total += r.getFee();
    }
    return total;
}

void ParkingLot::addToWaitingQueue(const std::string& plate, VehicleType type) {
    waitingQueue_.push({plate, type});
    LOG_INFO("车辆 " + plate + " 加入等待队列，当前等待: " + 
             std::to_string(waitingQueue_.size()) + " 辆");
}

bool ParkingLot::processWaitingQueue() {
    if (waitingQueue_.empty()) return false;
    
    ParkingSpot* spot = findAvailableSpot();
    if (!spot) return false;
    
    auto [plate, type] = waitingQueue_.front();
    waitingQueue_.pop();
    
    return enterParking(plate, type);
}

void ParkingLot::displayAllFloors() const {
    std::cout << "\n" << Color::CYAN 
              << "╔══════════════════════════════════════════════════════════════╗\n"
              << "║                    🚗 停车场实时状态                          ║\n"
              << "╚══════════════════════════════════════════════════════════════╝"
              << Color::RESET << "\n";
    
    for (const auto& floor : floors_) {
        floor.displayLayout();
    }
    
    std::cout << "\n" << Color::YELLOW << "图例: " << Color::RESET
              << Color::GREEN << "[□]空闲  " << Color::RESET
              << Color::RED << "[■]占用  " << Color::RESET
              << Color::YELLOW << "[◆]预约  " << Color::RESET
              << Color::MAGENTA << "[✕]维护" << Color::RESET << "\n";
}

void ParkingLot::displayStatistics() const {
    std::cout << "\n" << Color::CYAN
              << "╔══════════════════════════════════════════════════════════════╗\n"
              << "║                    📊 停车场统计信息                          ║\n"
              << "╚══════════════════════════════════════════════════════════════╝"
              << Color::RESET << "\n\n";
    
    std::cout << "  总车位数: " << Color::WHITE << getTotalSpots() << Color::RESET << "\n";
    std::cout << "  空闲车位: " << Color::GREEN << getAvailableCount() << Color::RESET << "\n";
    std::cout << "  占用车位: " << Color::RED << getOccupiedCount() << Color::RESET << "\n";
    std::cout << "  等待车辆: " << Color::YELLOW << waitingQueue_.size() << Color::RESET << "\n";
    std::cout << "  在场车辆: " << Color::BLUE << parkedVehicles_.size() << Color::RESET << "\n";
    std::cout << "  会员数量: " << Color::MAGENTA << members_.size() << Color::RESET << "\n";
    
    std::cout << "\n  各层统计:\n";
    for (const auto& floor : floors_) {
        std::cout << "    " << floor.getFloorId() << "层(" << floor.getZoneName() << "区): "
                  << Color::GREEN << floor.getAvailableCount() << Color::RESET << "/"
                  << floor.getTotalSpots() << " 空闲\n";
    }
}

void ParkingLot::displayPath(const std::vector<std::string>& path) const {
    if (path.empty()) {
        std::cout << Color::RED << "  无法计算路径" << Color::RESET << "\n";
        return;
    }
    
    std::cout << "\n" << Color::CYAN << "  🚗 最优行驶路线:" << Color::RESET << "\n  ";
    for (size_t i = 0; i < path.size(); ++i) {
        std::cout << Color::GREEN << path[i] << Color::RESET;
        if (i < path.size() - 1) {
            std::cout << " → ";
        }
    }
    std::cout << "\n";
}

void ParkingLot::cleanExpiredReservations() {
    for (auto& floor : floors_) {
        floor.cleanExpiredReservations();
    }
}

Floor* ParkingLot::getFloor(int index) {
    if (index < 0 || index >= static_cast<int>(floors_.size())) {
        return nullptr;
    }
    return &floors_[index];
}

// 生成临时文件名
std::string generateTempFilename(const std::string& basePath) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);
    return basePath + ".tmp." + std::to_string(dis(gen));
}

// 原子写入文件：先写入临时文件，成功后重命名
bool atomicWriteFile(const std::string& path, const std::string& content) {
    std::string tempPath = generateTempFilename(path);
    
    // 写入临时文件
    std::ofstream tempFile(tempPath);
    if (!tempFile.is_open()) {
        LOG_ERROR("无法创建临时文件: " + tempPath);
        return false;
    }
    
    tempFile << content;
    tempFile.flush();
    
    // 检查写入是否成功
    if (tempFile.fail()) {
        tempFile.close();
        std::filesystem::remove(tempPath);
        LOG_ERROR("写入临时文件失败: " + tempPath);
        return false;
    }
    
    tempFile.close();
    
    // 原子重命名
    std::error_code ec;
    std::filesystem::rename(tempPath, path, ec);
    if (ec) {
        std::filesystem::remove(tempPath);
        LOG_ERROR("重命名文件失败: " + ec.message());
        return false;
    }
    
    return true;
}

void ParkingLot::saveData() {
    // 使用 std::filesystem 创建数据目录
    std::error_code ec;
    std::filesystem::create_directories("data", ec);
    if (ec) {
        LOG_ERROR("创建数据目录失败: " + ec.message());
        return;
    }
    
    // 保存费率配置（原子写入）
    std::stringstream configContent;
    configContent << feeRate_.serialize() << "\n";
    if (!atomicWriteFile("data/config.dat", configContent.str())) {
        LOG_ERROR("保存费率配置失败");
    }
    
    // 保存会员数据（原子写入）
    std::stringstream memberContent;
    for (const auto& m : members_) {
        memberContent << m.serialize() << "\n";
    }
    if (!atomicWriteFile("data/members.dat", memberContent.str())) {
        LOG_ERROR("保存会员数据失败");
    }
    
    // 保存停车记录（原子写入）
    std::stringstream recordContent;
    for (const auto& r : records_) {
        recordContent << r.serialize() << "\n";
    }
    if (!atomicWriteFile("data/records.dat", recordContent.str())) {
        LOG_ERROR("保存停车记录失败");
    }
    
    // 保存预约数据（原子写入）
    std::stringstream reserveContent;
    for (const auto& r : reservations_) {
        reserveContent << r.serialize() << "\n";
    }
    if (!atomicWriteFile("data/reservations.dat", reserveContent.str())) {
        LOG_ERROR("保存预约数据失败");
    }
    
    LOG_INFO("数据已保存");
}

void ParkingLot::loadData() {
    std::string line;
    
    // 加载费率配置
    std::ifstream configFile("data/config.dat");
    if (configFile.is_open()) {
        if (std::getline(configFile, line) && !line.empty()) {
            feeRate_ = FeeRate::deserialize(line);
        }
        configFile.close();
    }
    
    // 加载会员数据
    std::ifstream memberFile("data/members.dat");
    if (memberFile.is_open()) {
        while (std::getline(memberFile, line)) {
            if (!line.empty()) {
                members_.push_back(Member::deserialize(line));
            }
        }
        memberFile.close();
    }
    
    // 加载停车记录
    std::ifstream recordFile("data/records.dat");
    if (recordFile.is_open()) {
        while (std::getline(recordFile, line)) {
            if (!line.empty()) {
                records_.push_back(ParkingRecord::deserialize(line));
            }
        }
        recordFile.close();
    }
    
    // 加载预约数据
    std::ifstream reserveFile("data/reservations.dat");
    if (reserveFile.is_open()) {
        while (std::getline(reserveFile, line)) {
            if (!line.empty()) {
                reservations_.push_back(Reservation::deserialize(line));
            }
        }
        reserveFile.close();
    }
    
    LOG_INFO("数据已加载");
}
