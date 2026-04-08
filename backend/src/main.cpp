#include "../include/parking_lot.h"
#include <csignal>

// 全局停车场实例
ParkingLot* g_parkingLot = nullptr;

// 安全输入辅助函数 - 处理非预期输入
template<typename T>
bool safeInput(T& value, const std::string& errorMsg = "无效输入，请重试") {
    std::cin >> value;
    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << Color::RED << errorMsg << Color::RESET << "\n";
        return false;
    }
    return true;
}

// 带范围检查的整数输入
bool safeInputInt(int& value, int minVal, int maxVal, const std::string& errorMsg = "无效输入，请重试") {
    if (!safeInput(value, errorMsg)) {
        return false;
    }
    if (value < minVal || value > maxVal) {
        std::cout << Color::RED << "输入超出范围 [" << minVal << "-" << maxVal << "]" << Color::RESET << "\n";
        return false;
    }
    return true;
}

// 带范围检查的浮点数输入
bool safeInputDouble(double& value, double minVal, double maxVal, const std::string& errorMsg = "无效输入，请重试") {
    if (!safeInput(value, errorMsg)) {
        return false;
    }
    if (value < minVal || value > maxVal) {
        std::cout << Color::RED << "输入超出范围 [" << minVal << "-" << maxVal << "]" << Color::RESET << "\n";
        return false;
    }
    return true;
}

// 信号处理
void signalHandler(int signum) {
    if (g_parkingLot) {
        g_parkingLot->saveData();
    }
    std::cout << "\n" << Color::YELLOW << "系统已安全退出" << Color::RESET << "\n";
    exit(signum);
}

// 显示主菜单
void displayMainMenu() {
    std::cout << "\n" << Color::CYAN
              << "╔══════════════════════════════════════════════════════════════╗\n"
              << "║              🚗 智能停车场管理系统 v1.0                        ║\n"
              << "╠══════════════════════════════════════════════════════════════╣\n"
              << "║  [1] 车辆入场    [2] 车辆出场    [3] 车位查询                  ║\n"
              << "║  [4] 预约车位    [5] VIP管理     [6] 管理员功能                ║\n"
              << "║  [7] 停车场地图  [8] 路径引导    [0] 退出系统                  ║\n"
              << "╚══════════════════════════════════════════════════════════════╝"
              << Color::RESET << "\n";
    std::cout << "\n请选择操作: ";
}

// 车辆入场
void handleVehicleEntry(ParkingLot& lot) {
    std::string plate;
    int typeChoice;
    
    std::cout << "\n" << Color::CYAN << "═══ 车辆入场 ═══" << Color::RESET << "\n";
    std::cout << "请输入车牌号: ";
    if (!safeInput(plate)) return;
    
    std::cout << "车辆类型 [1]轿车 [2]SUV [3]摩托车: ";
    if (!safeInputInt(typeChoice, 1, 3)) return;
    
    VehicleType type = VehicleType::CAR;
    switch (typeChoice) {
        case 2: type = VehicleType::SUV; break;
        case 3: type = VehicleType::MOTORCYCLE; break;
        default: type = VehicleType::CAR;
    }
    
    if (lot.enterParking(plate, type)) {
        Vehicle* v = lot.findVehicle(plate);
        std::cout << Color::GREEN << "\n✓ 入场成功！" << Color::RESET << "\n";
        std::cout << "  车牌号: " << plate << "\n";
        std::cout << "  车位号: " << v->getSpotId() << "\n";
        std::cout << "  入场时间: " << Utils::timeToStr(v->getEntryTime()) << "\n";
        
        // 显示路径引导
        auto path = lot.getOptimalPath(v->getSpotId());
        lot.displayPath(path);
    } else {
        if (lot.getAvailableCount() == 0) {
            std::cout << Color::YELLOW << "\n⚠ 车位已满，已加入等待队列" << Color::RESET << "\n";
            std::cout << "  当前等待: " << lot.getWaitingCount() << " 辆\n";
        } else {
            std::cout << Color::RED << "\n✗ 入场失败，请检查车牌是否已在场内" << Color::RESET << "\n";
        }
    }
}

// 车辆出场
void handleVehicleExit(ParkingLot& lot) {
    std::string plate;
    
    std::cout << "\n" << Color::CYAN << "═══ 车辆出场 ═══" << Color::RESET << "\n";
    std::cout << "请输入车牌号: ";
    if (!safeInput(plate)) return;
    
    Vehicle* v = lot.findVehicle(plate);
    if (!v) {
        std::cout << Color::RED << "\n✗ 未找到该车辆" << Color::RESET << "\n";
        return;
    }
    
    std::cout << "\n  车牌号: " << plate << "\n";
    std::cout << "  车位号: " << v->getSpotId() << "\n";
    std::cout << "  入场时间: " << Utils::timeToStr(v->getEntryTime()) << "\n";
    std::cout << "  停车时长: " << std::fixed << std::setprecision(2) 
              << v->getParkingDuration() << " 小时\n";
    
    Member* member = lot.getMember(plate);
    if (member) {
        std::cout << "  会员类型: " << Utils::memberTypeToStr(member->getType()) 
                  << " (折扣: " << (member->getDiscount() * 100) << "%)\n";
    }
    
    double fee = lot.exitParking(plate);
    if (fee >= 0) {
        std::cout << Color::GREEN << "\n✓ 出场成功！" << Color::RESET << "\n";
        std::cout << "  应付金额: " << Color::YELLOW << std::fixed << std::setprecision(2) 
                  << fee << " 元" << Color::RESET << "\n";
    } else {
        std::cout << Color::RED << "\n✗ 出场失败" << Color::RESET << "\n";
    }
}

// 车位查询
void handleSpotQuery(ParkingLot& lot) {
    std::cout << "\n" << Color::CYAN << "═══ 车位查询 ═══" << Color::RESET << "\n";
    std::cout << "[1] 查看所有空闲车位\n";
    std::cout << "[2] 查询指定车位状态\n";
    std::cout << "[3] 查询车辆位置\n";
    std::cout << "请选择: ";
    
    int choice;
    if (!safeInputInt(choice, 1, 3)) return;
    
    switch (choice) {
        case 1: {
            auto spots = lot.getAvailableSpots();
            std::cout << "\n空闲车位列表 (共 " << spots.size() << " 个):\n";
            for (size_t i = 0; i < spots.size(); ++i) {
                std::cout << spots[i]->getSpotId();
                if ((i + 1) % 10 == 0) std::cout << "\n";
                else std::cout << "  ";
            }
            std::cout << "\n";
            break;
        }
        case 2: {
            std::string spotId;
            std::cout << "请输入车位号: ";
            if (!safeInput(spotId)) break;
            ParkingSpot* spot = lot.getSpotById(spotId);
            if (spot) {
                std::cout << "\n车位 " << spotId << " 状态: " 
                          << Utils::spotStatusToStr(spot->getStatus()) << "\n";
                if (spot->getStatus() == SpotStatus::OCCUPIED && spot->getVehicle()) {
                    std::cout << "停放车辆: " << spot->getVehicle()->getLicensePlate() << "\n";
                }
            } else {
                std::cout << Color::RED << "未找到该车位" << Color::RESET << "\n";
            }
            break;
        }
        case 3: {
            std::string plate;
            std::cout << "请输入车牌号: ";
            if (!safeInput(plate)) break;
            Vehicle* v = lot.findVehicle(plate);
            if (v) {
                std::cout << "\n车辆 " << plate << " 停放于: " << v->getSpotId() << "\n";
            } else {
                std::cout << Color::RED << "未找到该车辆" << Color::RESET << "\n";
            }
            break;
        }
    }
}

// 预约车位
void handleReservation(ParkingLot& lot) {
    std::cout << "\n" << Color::CYAN << "═══ 预约车位 ═══" << Color::RESET << "\n";
    std::cout << "[1] 创建预约\n";
    std::cout << "[2] 取消预约\n";
    std::cout << "[3] 查询预约\n";
    std::cout << "请选择: ";
    
    int choice;
    if (!safeInputInt(choice, 1, 3)) return;
    
    switch (choice) {
        case 1: {
            std::string plate, spotId;
            int minutes;
            std::cout << "请输入车牌号: ";
            if (!safeInput(plate)) break;
            
            // 显示可预约车位
            auto spots = lot.getAvailableSpots();
            std::cout << "\n可预约车位: ";
            for (size_t i = 0; i < std::min(spots.size(), (size_t)10); ++i) {
                std::cout << spots[i]->getSpotId() << " ";
            }
            std::cout << "\n请输入要预约的车位号: ";
            if (!safeInput(spotId)) break;
            std::cout << "预约时长(分钟，默认30): ";
            if (!safeInputInt(minutes, 1, 1440)) break;  // 最长24小时
            if (minutes <= 0) minutes = 30;
            
            std::string reservationId = lot.createReservation(plate, spotId, minutes);
            if (!reservationId.empty()) {
                std::cout << Color::GREEN << "\n✓ 预约成功！" << Color::RESET << "\n";
                std::cout << "  预约号: " << reservationId << "\n";
                std::cout << "  车位号: " << spotId << "\n";
                std::cout << "  有效期: " << minutes << " 分钟\n";
            } else {
                std::cout << Color::RED << "\n✗ 预约失败，车位不可用或已有预约" << Color::RESET << "\n";
            }
            break;
        }
        case 2: {
            std::string reservationId;
            std::cout << "请输入预约号: ";
            if (!safeInput(reservationId)) break;
            if (lot.cancelReservation(reservationId)) {
                std::cout << Color::GREEN << "\n✓ 预约已取消" << Color::RESET << "\n";
            } else {
                std::cout << Color::RED << "\n✗ 取消失败，预约不存在或已过期" << Color::RESET << "\n";
            }
            break;
        }
        case 3: {
            std::string plate;
            std::cout << "请输入车牌号: ";
            if (!safeInput(plate)) break;
            Reservation* r = lot.getReservation(plate);
            if (r) {
                std::cout << "\n预约信息:\n";
                std::cout << "  预约号: " << r->getReservationId() << "\n";
                std::cout << "  车位号: " << r->getSpotId() << "\n";
                std::cout << "  剩余时间: " << r->getRemainingMinutes() << " 分钟\n";
            } else {
                std::cout << Color::YELLOW << "\n无有效预约" << Color::RESET << "\n";
            }
            break;
        }
    }
}

// VIP管理
void handleVIPManagement(ParkingLot& lot) {
    std::cout << "\n" << Color::CYAN << "═══ VIP会员管理 ═══" << Color::RESET << "\n";
    std::cout << "[1] 注册会员\n";
    std::cout << "[2] 会员续费\n";
    std::cout << "[3] 查询会员\n";
    std::cout << "[4] 会员列表\n";
    std::cout << "请选择: ";
    
    int choice;
    if (!safeInputInt(choice, 1, 4)) return;
    
    switch (choice) {
        case 1: {
            std::string plate;
            int typeChoice, months;
            std::cout << "请输入车牌号: ";
            if (!safeInput(plate)) break;
            std::cout << "会员类型 [1]月卡 [2]年卡 [3]VIP: ";
            if (!safeInputInt(typeChoice, 1, 3)) break;
            
            MemberType type = MemberType::MONTHLY;
            switch (typeChoice) {
                case 2: type = MemberType::YEARLY; months = 12; break;
                case 3: type = MemberType::VIP; months = 12; break;
                default: type = MemberType::MONTHLY; months = 1;
            }
            
            if (typeChoice == 1) {
                std::cout << "购买月数: ";
                if (!safeInputInt(months, 1, 36)) break;  // 最长3年
            }
            
            std::string memberId = lot.registerMember(plate, type, months);
            if (!memberId.empty()) {
                std::cout << Color::GREEN << "\n✓ 注册成功！" << Color::RESET << "\n";
                std::cout << "  会员号: " << memberId << "\n";
                std::cout << "  会员类型: " << Utils::memberTypeToStr(type) << "\n";
                std::cout << "  有效期: " << months << " 个月\n";
            } else {
                std::cout << Color::RED << "\n✗ 注册失败，可能已是会员" << Color::RESET << "\n";
            }
            break;
        }
        case 2: {
            std::string memberId;
            int months;
            std::cout << "请输入会员号: ";
            if (!safeInput(memberId)) break;
            std::cout << "续费月数: ";
            if (!safeInputInt(months, 1, 36)) break;
            
            if (lot.renewMembership(memberId, months)) {
                Member* m = lot.getMemberById(memberId);
                std::cout << Color::GREEN << "\n✓ 续费成功！" << Color::RESET << "\n";
                std::cout << "  剩余天数: " << m->getRemainingDays() << " 天\n";
            } else {
                std::cout << Color::RED << "\n✗ 续费失败" << Color::RESET << "\n";
            }
            break;
        }
        case 3: {
            std::string plate;
            std::cout << "请输入车牌号: ";
            if (!safeInput(plate)) break;
            Member* m = lot.getMember(plate);
            if (m) {
                std::cout << "\n会员信息:\n";
                std::cout << "  会员号: " << m->getMemberId() << "\n";
                std::cout << "  车牌号: " << m->getLicensePlate() << "\n";
                std::cout << "  会员类型: " << Utils::memberTypeToStr(m->getType()) << "\n";
                std::cout << "  折扣: " << (m->getDiscount() * 100) << "%\n";
                std::cout << "  剩余天数: " << m->getRemainingDays() << " 天\n";
            } else {
                std::cout << Color::YELLOW << "\n未找到有效会员" << Color::RESET << "\n";
            }
            break;
        }
        case 4: {
            auto& members = lot.getMembers();
            std::cout << "\n会员列表 (共 " << members.size() << " 人):\n";
            std::cout << std::setw(10) << "会员号" << std::setw(12) << "车牌号" 
                      << std::setw(8) << "类型" << std::setw(10) << "剩余天数" << "\n";
            std::cout << std::string(40, '-') << "\n";
            for (const auto& m : members) {
                std::cout << std::setw(10) << m.getMemberId() 
                          << std::setw(12) << m.getLicensePlate()
                          << std::setw(8) << Utils::memberTypeToStr(m.getType())
                          << std::setw(10) << m.getRemainingDays() << "\n";
            }
            break;
        }
    }
}

// 管理员功能
void handleAdminFunctions(ParkingLot& lot) {
    std::cout << "\n" << Color::CYAN << "═══ 管理员功能 ═══" << Color::RESET << "\n";
    std::cout << "[1] 设置费率\n";
    std::cout << "[2] 查看停车记录\n";
    std::cout << "[3] 统计今日收入\n";
    std::cout << "[4] 统计总收入\n";
    std::cout << "[5] 查看统计信息\n";
    std::cout << "请选择: ";
    
    int choice;
    if (!safeInputInt(choice, 1, 5)) return;
    
    switch (choice) {
        case 1: {
            double hourly, dailyMax;
            int freeMinutes;
            std::cout << "当前费率: " << lot.getFeeRate().getHourlyRate() << " 元/小时\n";
            std::cout << "当前封顶: " << lot.getFeeRate().getDailyMax() << " 元/天\n";
            std::cout << "免费时长: " << lot.getFeeRate().getFreeMinutes() << " 分钟\n\n";
            
            std::cout << "新小时费率: ";
            if (!safeInputDouble(hourly, 0.1, 100.0)) break;
            std::cout << "新每日封顶: ";
            if (!safeInputDouble(dailyMax, 1.0, 1000.0)) break;
            std::cout << "新免费时长(分钟): ";
            if (!safeInputInt(freeMinutes, 0, 120)) break;
            
            lot.setFeeRate(hourly, dailyMax);
            lot.getFeeRate().setFreeMinutes(freeMinutes);
            std::cout << Color::GREEN << "\n✓ 费率设置成功" << Color::RESET << "\n";
            break;
        }
        case 2: {
            auto& records = lot.getAllRecords();
            std::cout << "\n停车记录 (最近20条):\n";
            std::cout << std::setw(10) << "记录号" << std::setw(12) << "车牌号" 
                      << std::setw(6) << "车位" << std::setw(10) << "费用" << "\n";
            std::cout << std::string(40, '-') << "\n";
            
            int count = 0;
            for (auto it = records.rbegin(); it != records.rend() && count < 20; ++it, ++count) {
                std::cout << std::setw(10) << it->getRecordId()
                          << std::setw(12) << it->getLicensePlate()
                          << std::setw(6) << it->getSpotId()
                          << std::setw(10) << std::fixed << std::setprecision(2) 
                          << it->getFee() << "\n";
            }
            break;
        }
        case 3: {
            double income = lot.getDailyIncome(time(nullptr));
            std::cout << "\n今日收入: " << Color::GREEN << std::fixed << std::setprecision(2) 
                      << income << " 元" << Color::RESET << "\n";
            break;
        }
        case 4: {
            double income = lot.getTotalIncome();
            std::cout << "\n总收入: " << Color::GREEN << std::fixed << std::setprecision(2) 
                      << income << " 元" << Color::RESET << "\n";
            break;
        }
        case 5: {
            lot.displayStatistics();
            break;
        }
    }
}

// 路径引导
void handlePathGuide(ParkingLot& lot) {
    std::cout << "\n" << Color::CYAN << "═══ 路径引导 ═══" << Color::RESET << "\n";
    std::cout << "[1] 引导至最近空车位\n";
    std::cout << "[2] 引导至指定车位\n";
    std::cout << "请选择: ";
    
    int choice;
    if (!safeInputInt(choice, 1, 2)) return;
    
    switch (choice) {
        case 1: {
            auto path = lot.getPathToNearestSpot();
            if (path.empty()) {
                std::cout << Color::RED << "\n没有可用车位" << Color::RESET << "\n";
            } else {
                std::cout << "\n目标车位: " << path.back() << "\n";
                lot.displayPath(path);
            }
            break;
        }
        case 2: {
            std::string spotId;
            std::cout << "请输入目标车位号: ";
            if (!safeInput(spotId)) break;
            
            ParkingSpot* spot = lot.getSpotById(spotId);
            if (!spot) {
                std::cout << Color::RED << "\n车位不存在" << Color::RESET << "\n";
                break;
            }
            
            auto path = lot.getOptimalPath(spotId);
            if (path.empty()) {
                std::cout << Color::RED << "\n无法计算路径" << Color::RESET << "\n";
            } else {
                lot.displayPath(path);
            }
            break;
        }
    }
}

int main() {
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // 创建停车场
    ParkingLot lot("智能停车场");
    g_parkingLot = &lot;
    
    // 初始化：3层，每层2行5列（共30个车位）
    lot.initialize(3, 2, 5);
    
    // 加载历史数据
    lot.loadData();
    
    std::cout << Color::GREEN << "\n系统启动成功！" << Color::RESET << "\n";
    std::cout << "停车场: " << lot.getName() << "\n";
    std::cout << "总车位: " << lot.getTotalSpots() << " 个\n";
    
    int choice;
    while (true) {
        displayMainMenu();
        std::cin >> choice;
        
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << Color::RED << "无效输入，请重试" << Color::RESET << "\n";
            continue;
        }
        
        switch (choice) {
            case 1:
                handleVehicleEntry(lot);
                break;
            case 2:
                handleVehicleExit(lot);
                break;
            case 3:
                handleSpotQuery(lot);
                break;
            case 4:
                handleReservation(lot);
                break;
            case 5:
                handleVIPManagement(lot);
                break;
            case 6:
                handleAdminFunctions(lot);
                break;
            case 7:
                lot.displayAllFloors();
                break;
            case 8:
                handlePathGuide(lot);
                break;
            case 0:
                lot.saveData();
                std::cout << Color::GREEN << "\n感谢使用，再见！" << Color::RESET << "\n";
                return 0;
            default:
                std::cout << Color::RED << "无效选项，请重试" << Color::RESET << "\n";
        }
    }
    
    return 0;
}
