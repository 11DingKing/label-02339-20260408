#!/bin/bash
# 环境监测系统 - 测试运行脚本
# 用法: ./run_tests.sh [CMAKE_PREFIX_PATH]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build-test"
CMAKE_PREFIX=""

if [ -n "$1" ]; then
    CMAKE_PREFIX="-DCMAKE_PREFIX_PATH=$1"
fi

echo "========================================="
echo "  环境监测系统 - 自动化测试"
echo "========================================="
echo ""

# 清理并创建构建目录
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

echo "[1/3] 配置项目..."
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" ${CMAKE_PREFIX} -DCMAKE_BUILD_TYPE=Debug 2>&1

echo ""
echo "[2/3] 编译项目及测试..."
cmake --build "${BUILD_DIR}" -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) 2>&1

echo ""
echo "[3/3] 运行测试..."
echo "-----------------------------------------"

cd "${BUILD_DIR}"
ctest --output-on-failure --verbose 2>&1

echo ""
echo "========================================="
echo "  全部测试完成"
echo "========================================="
