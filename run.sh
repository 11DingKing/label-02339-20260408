#!/bin/bash
#
# 环境监测系统 - 一键启动脚本
# 支持 macOS 和 Linux，自动检测并安装依赖
#
# 使用方法:
#   chmod +x run.sh
#   ./run.sh
#

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKEND_DIR="${SCRIPT_DIR}/backend"
BUILD_DIR="${BACKEND_DIR}/build"

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[信息]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[成功]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[警告]${NC} $1"
}

print_error() {
    echo -e "${RED}[错误]${NC} $1"
}

# 检测操作系统
detect_os() {
    case "$(uname -s)" in
        Darwin*)
            OS="macos"
            ;;
        Linux*)
            OS="linux"
            # 检测发行版
            if [ -f /etc/os-release ]; then
                . /etc/os-release
                DISTRO=$ID
            fi
            ;;
        MINGW*|MSYS*|CYGWIN*)
            OS="windows"
            ;;
        *)
            OS="unknown"
            ;;
    esac
    echo "$OS"
}

# 检查命令是否存在
command_exists() {
    command -v "$1" &> /dev/null
}

# macOS 安装依赖
install_macos_deps() {
    print_info "检测到 macOS 系统"
    
    # 检查 Homebrew
    if ! command_exists brew; then
        print_warning "未检测到 Homebrew，正在安装..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        
        # 添加到 PATH (Apple Silicon)
        if [ -f "/opt/homebrew/bin/brew" ]; then
            eval "$(/opt/homebrew/bin/brew shellenv)"
        fi
    fi
    print_success "Homebrew 已就绪"
    
    # 检查 Qt6
    if ! brew list qt@6 &> /dev/null && ! brew list qt &> /dev/null; then
        print_warning "未检测到 Qt6，正在安装（可能需要几分钟）..."
        brew install qt@6
    fi
    print_success "Qt6 已就绪"
    
    # 检查 CMake
    if ! command_exists cmake; then
        print_warning "未检测到 CMake，正在安装..."
        brew install cmake
    fi
    print_success "CMake 已就绪"
    
    # 获取 Qt 路径
    if brew list qt@6 &> /dev/null; then
        QT_PATH="$(brew --prefix qt@6)"
    else
        QT_PATH="$(brew --prefix qt)"
    fi
    echo "$QT_PATH"
}

# Linux 安装依赖
install_linux_deps() {
    print_info "检测到 Linux 系统 (${DISTRO:-unknown})"
    
    case "$DISTRO" in
        ubuntu|debian|linuxmint|pop)
            # Debian/Ubuntu 系列
            if ! dpkg -l | grep -q "qt6-base-dev"; then
                print_warning "正在安装 Qt6 和依赖..."
                sudo apt-get update
                sudo apt-get install -y \
                    build-essential \
                    cmake \
                    sqlite3 \
                    qt6-base-dev \
                    qt6-serialport-dev \
                    libqt6sql6-sqlite \
                    libgl1-mesa-dev
            fi
            QT_PATH="/usr"
            ;;
        fedora|rhel|centos)
            # Fedora/RHEL 系列
            if ! rpm -q qt6-qtbase-devel &> /dev/null; then
                print_warning "正在安装 Qt6 和依赖..."
                sudo dnf install -y \
                    gcc-c++ \
                    cmake \
                    sqlite \
                    qt6-qtbase-devel \
                    qt6-qtserialport-devel \
                    mesa-libGL-devel
            fi
            QT_PATH="/usr"
            ;;
        arch|manjaro)
            # Arch 系列
            if ! pacman -Q qt6-base &> /dev/null; then
                print_warning "正在安装 Qt6 和依赖..."
                sudo pacman -S --noconfirm \
                    base-devel \
                    cmake \
                    sqlite \
                    qt6-base \
                    qt6-serialport
            fi
            QT_PATH="/usr"
            ;;
        *)
            print_error "不支持的 Linux 发行版: ${DISTRO:-unknown}"
            print_info "请手动安装: cmake, qt6-base-dev, qt6-serialport-dev"
            exit 1
            ;;
    esac
    
    print_success "依赖已就绪"
    echo "$QT_PATH"
}

# 初始化数据库
init_database() {
    local db_path="$1"
    local schema_file="${BACKEND_DIR}/schema.sql"
    local data_file="${BACKEND_DIR}/init_data.sql"
    
    if [ -f "$db_path" ]; then
        print_info "数据库已存在: $db_path"
        return 0
    fi
    
    print_info "初始化数据库..."
    
    # 创建目录
    mkdir -p "$(dirname "$db_path")"
    
    # 创建表结构
    if [ -f "$schema_file" ]; then
        sqlite3 "$db_path" < "$schema_file"
        print_success "数据库表结构已创建"
    fi
    
    # 导入测试数据
    if [ -f "$data_file" ]; then
        sqlite3 "$db_path" < "$data_file"
        local count=$(sqlite3 "$db_path" "SELECT COUNT(*) FROM environment_data;")
        print_success "已导入 $count 条测试数据"
    fi
}

# 编译项目
build_project() {
    local qt_path="$1"
    
    print_info "开始编译项目..."
    
    # 创建构建目录
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # CMake 配置
    print_info "运行 CMake 配置..."
    if [ -n "$qt_path" ] && [ "$qt_path" != "/usr" ]; then
        cmake .. -DCMAKE_PREFIX_PATH="$qt_path" -DCMAKE_BUILD_TYPE=Release
    else
        cmake .. -DCMAKE_BUILD_TYPE=Release
    fi
    
    # 编译
    print_info "编译中（使用 $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2) 核心）..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
    
    print_success "编译完成"
}

# 启动测试服务器（后台运行）
start_test_server() {
    local server_script="${SCRIPT_DIR}/test_server.py"
    
    if [ ! -f "$server_script" ]; then
        print_warning "测试服务器脚本不存在，跳过"
        return 0
    fi
    
    # 检查 Python3
    if ! command_exists python3; then
        print_warning "未检测到 Python3，跳过测试服务器"
        print_info "如需测试网络上传功能，请安装 Python3 后手动运行: python3 test_server.py"
        return 0
    fi
    
    # 检查端口是否已被占用
    if lsof -i:9000 &>/dev/null || netstat -an 2>/dev/null | grep -q ":9000.*LISTEN"; then
        print_info "端口 9000 已被占用，测试服务器可能已在运行"
        return 0
    fi
    
    print_info "启动测试服务器（端口 9000）..."
    python3 "$server_script" 9000 &
    TEST_SERVER_PID=$!
    sleep 1
    
    if kill -0 $TEST_SERVER_PID 2>/dev/null; then
        print_success "测试服务器已启动 (PID: $TEST_SERVER_PID)"
        print_info "网络上传测试: 服务器地址 127.0.0.1，端口 9000"
    else
        print_warning "测试服务器启动失败，网络上传功能需手动测试"
    fi
}

# 清理测试服务器
cleanup_test_server() {
    if [ -n "$TEST_SERVER_PID" ] && kill -0 $TEST_SERVER_PID 2>/dev/null; then
        print_info "关闭测试服务器..."
        kill $TEST_SERVER_PID 2>/dev/null || true
    fi
}

# 运行程序
run_app() {
    local app_path="${BUILD_DIR}/EnvMonitor"
    
    if [ ! -f "$app_path" ]; then
        print_error "找不到可执行文件: $app_path"
        exit 1
    fi
    
    print_success "启动环境监测系统..."
    echo ""
    echo "=========================================="
    echo "  环境监测系统 EnvMonitor v1.0"
    echo "=========================================="
    echo "  测试服务器: 127.0.0.1:9000 (TCP/UDP)"
    echo "=========================================="
    echo ""
    
    cd "$BUILD_DIR"
    
    # macOS 需要设置库路径
    if [ "$OS" = "macos" ]; then
        export DYLD_FRAMEWORK_PATH="$QT_PATH/lib"
    fi
    
    # 设置退出时清理
    trap cleanup_test_server EXIT
    
    ./EnvMonitor
}

# 主流程
main() {
    echo ""
    echo "=========================================="
    echo "  环境监测系统 - 一键启动"
    echo "=========================================="
    echo ""
    
    # 检测操作系统
    OS=$(detect_os)
    
    if [ "$OS" = "unknown" ]; then
        print_error "不支持的操作系统"
        exit 1
    fi
    
    if [ "$OS" = "windows" ]; then
        print_error "Windows 请使用 run.bat 脚本"
        print_info "或在 Git Bash/MSYS2 中运行本脚本"
        exit 1
    fi
    
    # 安装依赖
    case "$OS" in
        macos)
            QT_PATH=$(install_macos_deps)
            ;;
        linux)
            QT_PATH=$(install_linux_deps)
            ;;
    esac
    
    echo ""
    
    # 检查是否需要重新编译
    local need_rebuild=false
    local app_path="${BUILD_DIR}/EnvMonitor"
    
    if [ ! -f "$app_path" ]; then
        need_rebuild=true
        print_info "未检测到编译产物，需要编译"
    elif [ "$1" = "--rebuild" ]; then
        need_rebuild=true
        print_info "强制重新编译"
    else
        # 检查源代码是否有更新（比较最新源文件和可执行文件的修改时间）
        local newest_src=$(find "${BACKEND_DIR}/src" -name "*.cpp" -o -name "*.h" 2>/dev/null | xargs ls -t 2>/dev/null | head -1)
        if [ -n "$newest_src" ] && [ "$newest_src" -nt "$app_path" ]; then
            need_rebuild=true
            print_info "检测到源代码更新，需要重新编译"
        fi
    fi
    
    if [ "$need_rebuild" = true ]; then
        build_project "$QT_PATH"
    else
        print_success "代码无更新，跳过编译（使用 --rebuild 强制重新编译）"
    fi
    
    echo ""
    
    # 初始化数据库
    init_database "${BUILD_DIR}/envmonitor.db"
    
    echo ""
    
    # 启动测试服务器
    start_test_server
    
    echo ""
    
    # 运行程序
    run_app
}

# 显示帮助
show_help() {
    echo "环境监测系统 - 一键启动脚本"
    echo ""
    echo "用法: ./run.sh [选项]"
    echo ""
    echo "选项:"
    echo "  --rebuild    强制重新编译"
    echo "  --help       显示此帮助信息"
    echo ""
    echo "支持的系统:"
    echo "  - macOS (通过 Homebrew 安装依赖)"
    echo "  - Ubuntu/Debian"
    echo "  - Fedora/RHEL/CentOS"
    echo "  - Arch/Manjaro"
    echo ""
}

# 入口
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    show_help
    exit 0
fi

main "$@"
