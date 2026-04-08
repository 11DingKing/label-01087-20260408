#ifndef FLOOR_H
#define FLOOR_H

#include "common.h"
#include "parking_spot.h"

class Floor {
public:
    Floor() : floorId_(0), rows_(0), cols_(0) {}
    
    Floor(int floorId, const std::string& zoneName, int rows, int cols)
        : floorId_(floorId), zoneName_(zoneName), rows_(rows), cols_(cols) {
        spots_.resize(rows, std::vector<ParkingSpot>(cols));
    }
    
    // 初始化车位
    void initSpots(int startNodeId) {
        int nodeId = startNodeId;
        for (int i = 0; i < rows_; ++i) {
            for (int j = 0; j < cols_; ++j) {
                std::string spotId = zoneName_ + std::to_string(i * cols_ + j + 1);
                if (spotId.length() < 3) {
                    spotId = zoneName_ + "0" + std::to_string(i * cols_ + j + 1);
                }
                spots_[i][j] = ParkingSpot(spotId, j, i, nodeId++);
            }
        }
    }
    
    // Getters
    int getFloorId() const { return floorId_; }
    std::string getZoneName() const { return zoneName_; }
    int getRows() const { return rows_; }
    int getCols() const { return cols_; }
    int getTotalSpots() const { return rows_ * cols_; }
    
    // 获取车位
    ParkingSpot* getSpot(int row, int col) {
        if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
            return nullptr;
        }
        return &spots_[row][col];
    }
    
    ParkingSpot* getSpotById(const std::string& spotId) {
        for (auto& row : spots_) {
            for (auto& spot : row) {
                if (spot.getSpotId() == spotId) {
                    return &spot;
                }
            }
        }
        return nullptr;
    }
    
    // 获取空闲车位数量
    int getAvailableCount() const {
        int count = 0;
        for (const auto& row : spots_) {
            for (const auto& spot : row) {
                if (spot.getStatus() == SpotStatus::AVAILABLE) {
                    ++count;
                }
            }
        }
        return count;
    }
    
    // 获取所有空闲车位
    std::vector<ParkingSpot*> getAvailableSpots() {
        std::vector<ParkingSpot*> available;
        for (auto& row : spots_) {
            for (auto& spot : row) {
                if (spot.getStatus() == SpotStatus::AVAILABLE) {
                    available.push_back(&spot);
                }
            }
        }
        return available;
    }
    
    // 获取所有空闲车位的节点ID
    std::vector<int> getAvailableSpotNodeIds() const {
        std::vector<int> nodeIds;
        for (const auto& row : spots_) {
            for (const auto& spot : row) {
                if (spot.getStatus() == SpotStatus::AVAILABLE) {
                    nodeIds.push_back(spot.getNodeId());
                }
            }
        }
        return nodeIds;
    }
    
    // 显示楼层布局
    void displayLayout() const {
        std::cout << "\n" << Color::CYAN << "═══════════════ " 
                  << floorId_ << "层 " << zoneName_ << "区 ═══════════════" 
                  << Color::RESET << "\n";
        
        // 顶部边框
        std::cout << "┌";
        for (int j = 0; j < cols_; ++j) {
            std::cout << "─────" << (j < cols_ - 1 ? "┬" : "┐");
        }
        std::cout << "\n";
        
        for (int i = 0; i < rows_; ++i) {
            // 车位ID行
            std::cout << "│";
            for (int j = 0; j < cols_; ++j) {
                std::cout << " " << std::setw(3) << spots_[i][j].getSpotId() << " │";
            }
            std::cout << "\n";
            
            // 状态行
            std::cout << "│";
            for (int j = 0; j < cols_; ++j) {
                std::cout << " [" << spots_[i][j].getColoredDisplayChar() << "] │";
            }
            std::cout << "\n";
            
            // 分隔线
            if (i < rows_ - 1) {
                std::cout << "├";
                for (int j = 0; j < cols_; ++j) {
                    std::cout << "─────" << (j < cols_ - 1 ? "┼" : "┤");
                }
                std::cout << "\n";
            }
        }
        
        // 底部边框
        std::cout << "└";
        for (int j = 0; j < cols_; ++j) {
            std::cout << "─────" << (j < cols_ - 1 ? "┴" : "┘");
        }
        std::cout << "\n";
    }
    
    // 检查并清理过期预约
    void cleanExpiredReservations() {
        for (auto& row : spots_) {
            for (auto& spot : row) {
                if (spot.isReservationExpired()) {
                    spot.cancelReservation();
                    LOG_INFO("车位 " + spot.getSpotId() + " 预约已过期，自动取消");
                }
            }
        }
    }
    
    // 获取所有车位（用于遍历）
    std::vector<std::vector<ParkingSpot>>& getSpots() { return spots_; }
    const std::vector<std::vector<ParkingSpot>>& getSpots() const { return spots_; }
    
private:
    int floorId_;                                    // 楼层ID
    std::string zoneName_;                           // 区域名称
    int rows_, cols_;                                // 行列数
    std::vector<std::vector<ParkingSpot>> spots_;   // 车位二维数组
};

#endif // FLOOR_H
