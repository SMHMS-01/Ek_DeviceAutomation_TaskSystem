#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# 默认构建类型
BUILD_TYPE="${1:-Debug}"
ENABLE_TESTS="${2:-ON}"

echo "📦 Building Device Automation Task System"
echo "  Build Type: $BUILD_TYPE"
echo "  Tests: $ENABLE_TESTS"
echo "  Project: $PROJECT_DIR"
echo ""

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置
echo "🔧 Configuring CMake..."
cmake \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DENABLE_COVERAGE="$([ "$BUILD_TYPE" = "Debug" ] && echo ON || echo OFF)" \
    -DENABLE_ASAN="$([ "$BUILD_TYPE" = "Debug" ] && echo OFF || echo OFF)" \
    "$PROJECT_DIR"

# 构建
echo "🔨 Building..."
make -j"$(nproc)"

# 测试
if [ "$ENABLE_TESTS" = "ON" ]; then
    echo "🧪 Running tests..."
    ctest --output-on-failure --verbose || true
fi

# 覆盖率报告（仅 Debug）
if [ "$BUILD_TYPE" = "Debug" ] && command -v lcov &> /dev/null; then
    echo "📊 Generating coverage report..."
    lcov --directory . --capture --output-file coverage.info
    lcov --remove coverage.info '/usr/*' '*/third_party/*' --output-file coverage.info
    genhtml coverage.info --output-directory coverage_report
    echo "✅ Coverage report: file://$BUILD_DIR/coverage_report/index.html"
fi

echo ""
echo "✅ Build complete!"
echo "  Binaries: $BUILD_DIR/bin/"
echo "  Libraries: $BUILD_DIR/lib/"
