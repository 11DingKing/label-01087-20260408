# 智能停车场管理系统

## How to Run

### 方式一：Docker 运行（推荐）

```bash
# 构建并启动
docker-compose up --build -d

# 进入交互式终端
docker attach parking-system

# 退出时按 Ctrl+P, Ctrl+Q（保持容器运行）
```

### 方式二：本地编译运行

```bash
cd backend
mkdir -p build && cd build
cmake ..
make -j$(nproc)
./parking_system
```

### 方式三：直接编译（无需CMake）

```bash
cd backend
mkdir -p build
g++ -std=c++17 -Wall -I include src/parking_lot.cpp src/main.cpp -o build/parking_system
./build/parking_system
```

## Services

| 服务 | 端口 | 说明 |
|------|------|------|
| parking-system | - | C++ 控制台应用（交互式） |

## 测试账号

本系统为控制台交互程序，无需登录账号。

### 测试数据示例

- 车牌号：京A12345、京B67890、沪C11111
- 车位号：A01-A10（一层）、B01-B10（二层）、C01-C10（三层）

---

## 质检测试指南

### 运行单元测试

```bash
# 方式一：使用脚本
cd backend
./scripts/run_tests.sh

# 方式二：手动编译运行
cd backend
mkdir -p build
g++ -std=c++17 -Wall -I include src/parking_lot.cpp src/tests.cpp -o build/run_tests
./build/run_tests
```

### 预期测试结果

```
╔══════════════════════════════════════════════════════════════╗
║                    🧪 运行单元测试                            ║
╚══════════════════════════════════════════════════════════════╝

  ✓ Vehicle_Creation (0.01ms)
  ✓ Vehicle_ParkingDuration (0.00ms)
  ...
  ✓ ParkingLot_TotalIncome (0.05ms)

------------------------------------------------------------
总计: 50 个测试, 50 通过, 0 失败

✓ 所有测试通过！
```

### 运行交互式功能测试

```bash
cd backend
./scripts/interactive_test.sh
```

### 查看日志

```bash
cat backend/data/system.log
```

### 完整质检手册

详见 [docs/QA_MANUAL.md](docs/QA_MANUAL.md)

---

## 题目内容

### 智能停车场管理系统

**问题描述：** 模拟一个多层/多区域的停车场，实现车辆进出自动计费、车位引导和状态监控。

**基本功能要求：**

- **核心数据结构：** 停车场区域和车位可以使用二维数组或图来模拟布局，等待队列使用队列。
- **车辆管理：** 车辆入场（记录车牌、入场时间、分配车位）、车辆出场（根据时长计费、释放车位）。
- **车位管理：** 实时显示总车位、空余车位数量及位置（可简单用编号表示）。
- **管理员功能：** 设置费率、查看停车记录、统计每日收入。

**扩展功能：**

- **最优路径引导：** 使用图的最短路径算法（如Dijkstra算法），为车辆计算从入口到指定或最近空车位的最优行驶路线。
- **预约车位：** 用户可提前预约车位，预约信息需妥善管理。
- **VIP/长期用户管理：** 实现月卡、年卡用户的识别与计费。
- **数据可视化：** 用字符或简单图形在控制台模拟展示停车场各层车位占用状态。

---

## 功能说明

### 1. 车辆入场
- 输入车牌号和车辆类型
- 自动分配最近空闲车位
- 显示最优行驶路线（Dijkstra算法）
- 支持预约车位优先使用

### 2. 车辆出场
- 根据停车时长自动计费
- 支持会员折扣
- 自动处理等待队列

### 3. 车位管理
- 实时显示各层车位状态
- 字符图形化展示（■占用 □空闲 ◆预约 ✕维护）
- 支持按车位号/车牌号查询

### 4. 预约系统
- 提前预约指定车位
- 设置预约有效期
- 自动过期清理

### 5. VIP会员
- 月卡（8折）、年卡（6折）、VIP（5折）
- 会员注册、续费、查询

### 6. 管理员功能
- 设置小时费率、每日封顶、免费时长
- 查看停车记录
- 统计收入报表

### 7. 路径引导
- Dijkstra最短路径算法
- 引导至最近空车位
- 引导至指定车位

## 技术架构

```
backend/
├── include/           # 头文件
│   ├── common.h       # 公共定义、工具函数、日志系统
│   ├── vehicle.h      # 车辆类
│   ├── parking_spot.h # 车位类
│   ├── floor.h        # 楼层类
│   ├── parking_lot.h  # 停车场类
│   ├── graph.h        # 图结构（Dijkstra算法）
│   ├── member.h       # 会员类
│   ├── reservation.h  # 预约类
│   ├── fee_rate.h     # 费率类
│   └── parking_record.h # 停车记录类
├── src/               # 源文件
│   ├── main.cpp       # 主程序入口
│   └── parking_lot.cpp # 停车场实现
├── CMakeLists.txt     # CMake配置
└── Dockerfile         # Docker构建文件
```

## 数据结构

- **停车场布局：** 二维数组模拟每层车位
- **路径规划：** 邻接表图结构 + Dijkstra算法
- **等待队列：** STL queue
- **数据持久化：** 文件存储（CSV格式）

## 作者

智能停车场管理系统 v1.0
