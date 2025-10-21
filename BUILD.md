# 构建说明 - Browser Split Screen

## 快速开始

### 1. 检查WebEngine支持

首先检查您的Qt安装是否包含WebEngine支持：

```bash
./install_webengine.sh --check
```

如果显示"WebEngine未安装"，请运行：

```bash
./install_webengine.sh
```

### 2. 构建项目

```bash
# 清理并构建
./build.sh --clean --build

# 或者执行完整流程
./build.sh --all
```

## 解决方案说明

### 问题：Qt6WebEngineWidgets找不到

**错误信息：**
```
Failed to find required Qt component "WebEngineWidgets"
```

**原因：** 您的Qt安装中没有包含WebEngine模块。

**解决方案：**

#### 方案1：安装WebEngine支持（推荐）

**macOS：**
```bash
# 使用Homebrew安装完整Qt6
brew install qt@6

# 或使用Qt安装器添加WebEngine模块
# 1. 打开Qt安装器
# 2. 选择"添加或移除组件"
# 3. 展开Qt 6.9.1
# 4. 勾选"Qt WebEngine"
# 5. 完成安装
```

**Linux (Ubuntu/Debian)：**
```bash
sudo apt-get update
sudo apt-get install qt6-webengine-dev libqt6webenginewidgets6
```

**Linux (CentOS/RHEL)：**
```bash
sudo yum install qt6-qtwebengine-devel
```

#### 方案2：使用简化版本（无WebEngine）

如果无法安装WebEngine，项目已经配置为可以在没有WebEngine的情况下编译：

```bash
# 直接构建，会自动使用QTextBrowser作为替代
./build.sh --build
```

**功能限制：**
- 无法加载真实网页
- 只能显示简单的HTML内容
- 部分浏览器功能不可用

## 构建选项

### 使用构建脚本

```bash
# 显示帮助
./build.sh --help

# 安装依赖
./build.sh --deps

# 清理构建目录
./build.sh --clean

# 仅构建项目
./build.sh --build

# 运行测试
./build.sh --test

# 创建发布包
./build.sh --package

# 部署到系统
./build.sh --deploy

# 执行所有步骤
./build.sh --all
```

### 手动构建

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build . --parallel $(nproc)

# 运行
./BrowserSplitScreen
```

## 故障排除

### 1. CMake找不到Qt6

**解决方案：**
```bash
# 设置Qt6路径
export Qt6_DIR=/path/to/qt6/lib/cmake/Qt6

# 或在CMake命令中指定
cmake .. -DQt6_DIR=/path/to/qt6/lib/cmake/Qt6
```

### 2. 找不到SQLite3

**解决方案：**
```bash
# Ubuntu/Debian
sudo apt-get install libsqlite3-dev

# macOS
brew install sqlite3

# CentOS/RHEL
sudo yum install sqlite-devel
```

### 3. 编译错误

**常见问题：**
- 确保使用C++17兼容的编译器
- 检查Qt版本是否为6.9.1或更高
- 确保所有依赖库已正确安装

### 4. 运行时错误

**数据库权限问题：**
```bash
# 确保应用有写入权限
chmod 755 ~/.local/share/QunKong/
```

**WebEngine初始化失败：**
- 检查系统是否支持WebEngine
- 尝试使用简化版本（无WebEngine）

## 开发模式

### 调试构建

```bash
mkdir build-debug
cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### 启用详细日志

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DQT_DEBUG_FIND_PACKAGE=ON
```

### 代码格式化

```bash
# 使用clang-format
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

## 性能优化

### 发布构建

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### 减少二进制大小

```bash
# 使用strip移除调试符号
strip BrowserSplitScreen

# 使用UPX压缩（可选）
upx --best BrowserSplitScreen
```

## 跨平台构建

### Windows (使用WSL)

```bash
# 在WSL中安装依赖
sudo apt-get install qt6-base-dev qt6-webengine-dev cmake build-essential

# 构建
./build.sh --build
```

### macOS

```bash
# 安装依赖
brew install qt@6 cmake sqlite3

# 构建
./build.sh --build
```

### Linux

```bash
# Ubuntu/Debian
sudo apt-get install qt6-base-dev qt6-webengine-dev cmake build-essential libsqlite3-dev

# 构建
./build.sh --build
```

## 持续集成

### GitHub Actions

项目包含GitHub Actions配置，支持：
- 自动构建Windows、macOS、Linux版本
- 自动测试
- 自动创建发布包

### 本地CI测试

```bash
# 运行所有测试
./build.sh --test

# 检查代码质量
cppcheck --enable=all src/ include/
```

---

**注意：** 如果遇到其他问题，请查看完整的[部署文档](DEPLOYMENT.md)或提交Issue。
