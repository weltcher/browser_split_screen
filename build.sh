#!/bin/bash

# Browser Split Screen 构建脚本
# 支持 Linux、macOS 和 Windows (WSL)

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检测操作系统
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
    elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        OS="windows"
    else
        log_error "不支持的操作系统: $OSTYPE"
        exit 1
    fi
    log_info "检测到操作系统: $OS"
}

# 检查依赖
check_dependencies() {
    log_info "检查构建依赖..."
    
    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake 未安装，请先安装 CMake"
        exit 1
    fi
    
    # 检查Qt
    if ! command -v qmake &> /dev/null && ! command -v qmake6 &> /dev/null; then
        log_error "Qt 未安装，请先安装 Qt 6.9.1"
        exit 1
    fi
    
    # 检查编译器
    if [[ "$OS" == "linux" ]]; then
        if ! command -v g++ &> /dev/null; then
            log_error "GCC 未安装，请先安装 build-essential"
            exit 1
        fi
    elif [[ "$OS" == "macos" ]]; then
        if ! command -v clang++ &> /dev/null; then
            log_error "Clang 未安装，请先安装 Xcode Command Line Tools"
            exit 1
        fi
    fi
    
    log_success "所有依赖检查通过"
}

# 安装依赖 (Linux)
install_dependencies_linux() {
    log_info "安装 Linux 依赖..."
    
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y \
            qt6-base-dev \
            qt6-webengine-dev \
            cmake \
            build-essential \
            libsqlite3-dev \
            pkg-config
    elif command -v yum &> /dev/null; then
        sudo yum install -y \
            qt6-qtbase-devel \
            qt6-qtwebengine-devel \
            cmake \
            gcc-c++ \
            sqlite-devel
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y \
            qt6-qtbase-devel \
            qt6-qtwebengine-devel \
            cmake \
            gcc-c++ \
            sqlite-devel
    else
        log_warning "未识别的包管理器，请手动安装依赖"
    fi
}

# 安装依赖 (macOS)
install_dependencies_macos() {
    log_info "安装 macOS 依赖..."
    
    if command -v brew &> /dev/null; then
        brew install qt@6 cmake sqlite3
    else
        log_warning "Homebrew 未安装，请手动安装 Qt 6.9.1"
    fi
}

# 清理构建目录
clean_build() {
    log_info "清理构建目录..."
    
    if [ -d "build" ]; then
        rm -rf build
        log_success "构建目录已清理"
    fi
}

# 配置项目
configure_project() {
    log_info "配置项目..."
    
    mkdir -p build
    cd build
    
    # 设置Qt路径
    if [[ "$OS" == "macos" ]]; then
        export Qt6_DIR=$(brew --prefix qt@6)/lib/cmake/Qt6
    fi
    
    # 配置CMake
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local
    
    cd ..
    log_success "项目配置完成"
}

# 构建项目
build_project() {
    log_info "构建项目..."
    
    cd build
    
    # 获取CPU核心数
    if [[ "$OS" == "linux" ]]; then
        CORES=$(nproc)
    elif [[ "$OS" == "macos" ]]; then
        CORES=$(sysctl -n hw.ncpu)
    else
        CORES=4
    fi
    
    # 并行构建
    cmake --build . --parallel $CORES
    
    cd ..
    log_success "项目构建完成"
}

# 运行测试
run_tests() {
    log_info "运行测试..."
    
    cd build
    
    if [ -f "test/BrowserSplitScreen_test" ]; then
        ./test/BrowserSplitScreen_test
        log_success "测试通过"
    else
        log_warning "未找到测试文件"
    fi
    
    cd ..
}

# 安装应用程序
install_app() {
    log_info "安装应用程序..."
    
    cd build
    sudo cmake --install .
    cd ..
    
    log_success "应用程序安装完成"
}

# 创建发布包
create_package() {
    log_info "创建发布包..."
    
    PACKAGE_NAME="BrowserSplitScreen-$(date +%Y%m%d)-$OS"
    PACKAGE_DIR="packages/$PACKAGE_NAME"
    
    mkdir -p $PACKAGE_DIR
    
    # 复制可执行文件
    if [[ "$OS" == "macos" ]]; then
        cp build/BrowserSplitScreen.app/Contents/MacOS/BrowserSplitScreen $PACKAGE_DIR/
    else
        cp build/BrowserSplitScreen $PACKAGE_DIR/
    fi
    
    # 复制文档
    cp README.md $PACKAGE_DIR/
    cp DEPLOYMENT.md $PACKAGE_DIR/
    
    # 创建启动脚本
    if [[ "$OS" == "macos" ]]; then
        cat > $PACKAGE_DIR/run.sh << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
open -a BrowserSplitScreen "$@"
EOF
    else
        cat > $PACKAGE_DIR/run.sh << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
./BrowserSplitScreen "$@"
EOF
    fi
    chmod +x $PACKAGE_DIR/run.sh
    
    # 创建桌面文件 (Linux)
    if [[ "$OS" == "linux" ]]; then
        cat > $PACKAGE_DIR/browser-split-screen.desktop << EOF
[Desktop Entry]
Version=1.0.0
Type=Application
Name=Browser Split Screen
Comment=Multi-window browser tool
Exec=$(pwd)/BrowserSplitScreen
Icon=browser-split-screen
Terminal=false
Categories=Network;WebBrowser;
EOF
    fi
    
    # 创建压缩包
    cd packages
    tar -czf $PACKAGE_NAME.tar.gz $PACKAGE_NAME
    cd ..
    
    log_success "发布包已创建: packages/$PACKAGE_NAME.tar.gz"
}

# 部署到系统
deploy_system() {
    log_info "部署到系统..."
    
    if [[ "$OS" == "linux" ]]; then
        # 创建符号链接
        sudo ln -sf $(pwd)/build/BrowserSplitScreen /usr/local/bin/browser-split-screen
        
        # 复制桌面文件
        if [ -f "packages/*/browser-split-screen.desktop" ]; then
            sudo cp packages/*/browser-split-screen.desktop /usr/share/applications/
        fi
        
        log_success "已部署到系统"
    elif [[ "$OS" == "macos" ]]; then
        # 创建应用程序包
        mkdir -p BrowserSplitScreen.app/Contents/MacOS
        cp build/BrowserSplitScreen BrowserSplitScreen.app/Contents/MacOS/
        
        # 创建Info.plist
        cat > BrowserSplitScreen.app/Contents/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>BrowserSplitScreen</string>
    <key>CFBundleIdentifier</key>
    <string>com.qunkong.browsersplitscreen</string>
    <key>CFBundleName</key>
    <string>Browser Split Screen</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
</dict>
</plist>
EOF
        
        log_success "已创建应用程序包: BrowserSplitScreen.app"
    fi
}

# 显示帮助信息
show_help() {
    echo "Browser Split Screen 构建脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help          显示此帮助信息"
    echo "  -c, --clean         清理构建目录"
    echo "  -d, --deps          安装依赖"
    echo "  -b, --build         构建项目"
    echo "  -t, --test          运行测试"
    echo "  -i, --install       安装到系统"
    echo "  -p, --package       创建发布包"
    echo "  -D, --deploy        部署到系统"
    echo "  -a, --all           执行所有步骤"
    echo ""
    echo "示例:"
    echo "  $0 --all            # 执行完整构建流程"
    echo "  $0 --build          # 仅构建项目"
    echo "  $0 --clean --build  # 清理后构建"
}

# 主函数
main() {
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                CLEAN=true
                shift
                ;;
            -d|--deps)
                INSTALL_DEPS=true
                shift
                ;;
            -b|--build)
                BUILD=true
                shift
                ;;
            -t|--test)
                TEST=true
                shift
                ;;
            -i|--install)
                INSTALL=true
                shift
                ;;
            -p|--package)
                PACKAGE=true
                shift
                ;;
            -D|--deploy)
                DEPLOY=true
                shift
                ;;
            -a|--all)
                ALL=true
                shift
                ;;
            *)
                log_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 如果没有指定任何选项，显示帮助
    if [[ -z "$CLEAN" && -z "$INSTALL_DEPS" && -z "$BUILD" && -z "$TEST" && -z "$INSTALL" && -z "$PACKAGE" && -z "$DEPLOY" && -z "$ALL" ]]; then
        show_help
        exit 0
    fi
    
    # 检测操作系统
    detect_os
    
    # 执行所有步骤
    if [[ "$ALL" == "true" ]]; then
        check_dependencies
        clean_build
        configure_project
        build_project
        run_tests
        create_package
        deploy_system
        log_success "完整构建流程完成！"
        exit 0
    fi
    
    # 执行指定步骤
    if [[ "$CLEAN" == "true" ]]; then
        clean_build
    fi
    
    if [[ "$INSTALL_DEPS" == "true" ]]; then
        if [[ "$OS" == "linux" ]]; then
            install_dependencies_linux
        elif [[ "$OS" == "macos" ]]; then
            install_dependencies_macos
        fi
    fi
    
    if [[ "$BUILD" == "true" ]]; then
        check_dependencies
        configure_project
        build_project
    fi
    
    if [[ "$TEST" == "true" ]]; then
        run_tests
    fi
    
    if [[ "$INSTALL" == "true" ]]; then
        install_app
    fi
    
    if [[ "$PACKAGE" == "true" ]]; then
        create_package
    fi
    
    if [[ "$DEPLOY" == "true" ]]; then
        deploy_system
    fi
    
    log_success "构建完成！"
}

# 运行主函数
main "$@"

