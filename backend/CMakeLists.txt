cmake_minimum_required(VERSION 3.16)
project(ParkingLotSystem VERSION 1.0.0 LANGUAGES CXX)

# C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 源文件
set(SOURCES
    src/main.cpp
    src/parking_lot.cpp
)

# 可执行文件
add_executable(parking_system ${SOURCES})

# 链接 filesystem 库（GCC < 9 需要显式链接）
target_link_libraries(parking_system PRIVATE stdc++fs)

# 安装规则
install(TARGETS parking_system DESTINATION bin)

# 创建数据目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)
