cmake_minimum_required(VERSION 3.16)
project(ParkingLotSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/parking_lot.cpp
    src/http_server.cpp
)

add_executable(parking_system ${SOURCES})

if(APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(parking_system PRIVATE Threads::Threads)
else()
    target_link_libraries(parking_system PRIVATE stdc++fs)
endif()

install(TARGETS parking_system DESTINATION bin)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)
