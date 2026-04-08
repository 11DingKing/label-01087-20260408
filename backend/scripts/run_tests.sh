#!/bin/bash

# 智能停车场管理系统 - 自动化测试脚本
# 用法: ./scripts/run_tests.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo "========================================"
echo "  智能停车场管理系统 - 自动化测试"
echo "========================================"
echo ""

# 创建构建目录
mkdir -p "$BUILD_DIR"

# 编译测试程序
echo "[1/3] 编译测试程序..."
g++ -std=c++17 -Wall -Wextra -I "$PROJECT_DIR/include" \
    "$PROJECT_DIR/src/parking_lot.cpp" \
    "$PROJECT_DIR/src/tests.cpp" \
    -o "$BUILD_DIR/run_tests"

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi
echo "✓ 编译成功"
echo ""

# 创建数据目录
mkdir -p "$PROJECT_DIR/data"

# 运行测试
echo "[2/3] 运行单元测试..."
echo ""
"$BUILD_DIR/run_tests"
TEST_RESULT=$?

echo ""
echo "[3/3] 测试完成"

if [ $TEST_RESULT -eq 0 ]; then
    echo "========================================"
    echo "  ✓ 所有测试通过！"
    echo "========================================"
    exit 0
else
    echo "========================================"
    echo "  ✗ 存在失败的测试"
    echo "========================================"
    exit 1
fi
