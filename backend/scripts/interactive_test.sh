#!/bin/bash

# 智能停车场管理系统 - 交互式功能测试脚本
# 模拟用户操作，用于质检人员验证

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

echo "========================================"
echo "  智能停车场管理系统 - 交互式测试"
echo "========================================"
echo ""

# 编译主程序
echo "[编译主程序...]"
mkdir -p "$BUILD_DIR"
g++ -std=c++17 -Wall -I "$PROJECT_DIR/include" \
    "$PROJECT_DIR/src/parking_lot.cpp" \
    "$PROJECT_DIR/src/main.cpp" \
    -o "$BUILD_DIR/parking_system"

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi
echo "✓ 编译成功"
echo ""

# 创建测试输入
cat > /tmp/parking_test_input.txt << 'EOF'
1
京A12345
1
1
京B67890
2
1
京C11111
3
7
3
1
4
1
京D44444
A03
30
5
1
京E55555
1
3
5
4
6
5
8
1
2
京A12345
6
3
6
4
0
EOF

echo "开始自动化功能测试..."
echo "测试场景："
echo "  1. 车辆入场（轿车、SUV、摩托车）"
echo "  2. 查看停车场地图"
echo "  3. 查看空闲车位"
echo "  4. 预约车位"
echo "  5. 注册VIP会员"
echo "  6. 查看统计信息"
echo "  7. 路径引导"
echo "  8. 车辆出场"
echo "  9. 查看收入统计"
echo ""
echo "========================================"
echo ""

# 运行测试
"$BUILD_DIR/parking_system" < /tmp/parking_test_input.txt

echo ""
echo "========================================"
echo "  交互式测试完成"
echo "========================================"

# 清理
rm -f /tmp/parking_test_input.txt
