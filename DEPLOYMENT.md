# 部署指南 - Browser Split Screen

本文档详细说明如何在不同平台上部署Browser Split Screen应用程序。

## 目录
- [Windows部署](#windows部署)
- [macOS部署](#macos部署)
- [Linux部署](#linux部署)
- [Docker部署](#docker部署)
- [CI/CD自动化部署](#cicd自动化部署)

## Windows部署

### 方法一：使用Qt部署工具（推荐）

1. **构建Release版本**
```cmd
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

2. **使用windeployqt部署**
```cmd
# 找到windeployqt工具路径（通常在Qt安装目录下）
set QT_DIR=C:\Qt\6.9.1\msvc2019_64
%QT_DIR%\bin\windeployqt.exe --release --no-translations --no-system-d3d-compiler --no-opengl-sw BrowserSplitScreen.exe
```

3. **验证部署**
```cmd
# 检查依赖文件
dir /b *.dll
# 应该看到Qt相关的DLL文件

# 测试运行
BrowserSplitScreen.exe
```

### 方法二：手动部署

1. **复制必要文件**
```cmd
# 创建部署目录
mkdir BrowserSplitScreen_Deploy
cd BrowserSplitScreen_Deploy

# 复制可执行文件
copy ..\build\Release\BrowserSplitScreen.exe .

# 复制Qt运行时库
copy %QT_DIR%\bin\Qt6Core.dll .
copy %QT_DIR%\bin\Qt6Gui.dll .
copy %QT_DIR%\bin\Qt6Widgets.dll .
copy %QT_DIR%\bin\Qt6WebEngineWidgets.dll .
copy %QT_DIR%\bin\Qt6WebEngineCore.dll .
copy %QT_DIR%\bin\Qt6Sql.dll .
copy %QT_DIR%\bin\Qt6Network.dll .
copy %QT_DIR%\bin\Qt6WebChannel.dll .
copy %QT_DIR%\bin\Qt6Positioning.dll .
copy %QT_DIR%\bin\Qt6PrintSupport.dll .
copy %QT_DIR%\bin\Qt6Qml.dll .
copy %QT_DIR%\bin\Qt6Quick.dll .
copy %QT_DIR%\bin\Qt6QuickWidgets.dll .
copy %QT_DIR%\bin\Qt6Svg.dll .
copy %QT_DIR%\bin\Qt6OpenGL.dll .

# 复制平台插件
mkdir platforms
copy %QT_DIR%\plugins\platforms\qwindows.dll platforms\

# 复制WebEngine资源
mkdir resources
copy %QT_DIR%\resources\*.* resources\

# 复制翻译文件（可选）
mkdir translations
copy %QT_DIR%\translations\qt_*.qm translations\
```

2. **创建安装脚本**
```cmd
# 创建install.bat
@echo off
echo Installing Browser Split Screen...
xcopy /E /I /Y . "%PROGRAMFILES%\BrowserSplitScreen"
echo Installation completed!
pause
```

### 创建Windows安装包

使用NSIS创建安装程序：

1. **安装NSIS**
```cmd
# 下载并安装NSIS
# https://nsis.sourceforge.io/Download
```

2. **创建安装脚本 (installer.nsi)**
```nsis
!define APPNAME "Browser Split Screen"
!define COMPANYNAME "QunKong"
!define DESCRIPTION "Multi-window browser tool"
!define VERSIONMAJOR 1
!define VERSIONMINOR 0
!define VERSIONBUILD 0

!define HELPURL "https://github.com/qunkong/browser-split-screen"
!define UPDATEURL "https://github.com/qunkong/browser-split-screen/releases"
!define ABOUTURL "https://github.com/qunkong/browser-split-screen"

!define INSTALLSIZE 50000

RequestExecutionLevel admin
InstallDir "$PROGRAMFILES\${COMPANYNAME}\${APPNAME}"
Name "${APPNAME}"
outFile "BrowserSplitScreen_Setup.exe"

!include LogicLib.nsh

page directory
page instfiles

!macro VerifyUserIsAdmin
UserInfo::GetAccountType
pop $0
${If} $0 != "admin"
    messageBox mb_iconstop "Administrator rights required!"
    setErrorLevel 740
    quit
${EndIf}
!macroend

function .onInit
    setShellVarContext all
    !insertmacro VerifyUserIsAdmin
functionEnd

section "install"
    setOutPath $INSTDIR
    file /r "BrowserSplitScreen_Deploy\*.*"
    
    writeUninstaller "$INSTDIR\uninstall.exe"
    
    createDirectory "$SMPROGRAMS\${COMPANYNAME}"
    createShortCut "$SMPROGRAMS\${COMPANYNAME}\${APPNAME}.lnk" "$INSTDIR\BrowserSplitScreen.exe"
    createShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\BrowserSplitScreen.exe"
    
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "DisplayName" "${APPNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "InstallLocation" "$\"$INSTDIR$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "DisplayIcon" "$\"$INSTDIR\BrowserSplitScreen.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "Publisher" "${COMPANYNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "HelpLink" "${HELPURL}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "URLUpdateInfo" "${UPDATEURL}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "URLInfoAbout" "${ABOUTURL}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "DisplayVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "VersionMajor" ${VERSIONMAJOR}
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "VersionMinor" ${VERSIONMINOR}
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "NoRepair" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "EstimatedSize" ${INSTALLSIZE}
sectionEnd

section "uninstall"
    delete "$INSTDIR\uninstall.exe"
    rmDir /r "$INSTDIR"
    
    delete "$SMPROGRAMS\${COMPANYNAME}\${APPNAME}.lnk"
    delete "$DESKTOP\${APPNAME}.lnk"
    rmDir "$SMPROGRAMS\${COMPANYNAME}"
    
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}"
sectionEnd
```

3. **编译安装程序**
```cmd
makensis installer.nsi
```

## macOS部署

### 方法一：使用macdeployqt（推荐）

1. **构建应用程序**
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

2. **创建应用程序包**
```bash
# 创建.app包结构
mkdir -p BrowserSplitScreen.app/Contents/MacOS
mkdir -p BrowserSplitScreen.app/Contents/Resources
mkdir -p BrowserSplitScreen.app/Contents/Frameworks

# 复制可执行文件
cp BrowserSplitScreen BrowserSplitScreen.app/Contents/MacOS/

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
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSRequiresAquaSystemAppearance</key>
    <false/>
</dict>
</plist>
EOF
```

3. **使用macdeployqt部署**
```bash
# 找到macdeployqt工具
export QT_DIR=/opt/homebrew/lib/cmake/Qt6
$QT_DIR/../bin/macdeployqt BrowserSplitScreen.app

# 或使用完整路径
/opt/homebrew/bin/macdeployqt BrowserSplitScreen.app
```

### 方法二：手动部署

1. **复制Qt框架**
```bash
# 复制Qt框架到应用程序包
cp -R /opt/homebrew/lib/QtCore.framework BrowserSplitScreen.app/Contents/Frameworks/
cp -R /opt/homebrew/lib/QtGui.framework BrowserSplitScreen.app/Contents/Frameworks/
cp -R /opt/homebrew/lib/QtWidgets.framework BrowserSplitScreen.app/Contents/Frameworks/
cp -R /opt/homebrew/lib/QtWebEngineWidgets.framework BrowserSplitScreen.app/Contents/Frameworks/
cp -R /opt/homebrew/lib/QtWebEngineCore.framework BrowserSplitScreen.app/Contents/Frameworks/
cp -R /opt/homebrew/lib/QtSql.framework BrowserSplitScreen.app/Contents/Frameworks/
```

2. **修复库路径**
```bash
# 使用install_name_tool修复库路径
install_name_tool -change @rpath/QtCore.framework/Versions/A/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore BrowserSplitScreen.app/Contents/MacOS/BrowserSplitScreen
```

### 代码签名和公证

1. **开发者证书签名**
```bash
# 签名应用程序
codesign --force --verify --verbose --sign "Developer ID Application: Your Name" BrowserSplitScreen.app

# 验证签名
codesign --verify --verbose BrowserSplitScreen.app
spctl --assess --verbose BrowserSplitScreen.app
```

2. **创建DMG安装包**
```bash
# 创建DMG
hdiutil create -volname "Browser Split Screen" -srcfolder BrowserSplitScreen.app -ov -format UDZO BrowserSplitScreen.dmg

# 签名DMG
codesign --force --verify --verbose --sign "Developer ID Application: Your Name" BrowserSplitScreen.dmg
```

3. **公证（可选）**
```bash
# 上传到Apple公证服务
xcrun notarytool submit BrowserSplitScreen.dmg --keychain-profile "notarytool-profile" --wait

# 装订公证票据
xcrun stapler staple BrowserSplitScreen.dmg
```

## Linux部署

### 方法一：使用linuxdeployqt创建AppImage

1. **安装linuxdeployqt**
```bash
# 下载linuxdeployqt
wget https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod +x linuxdeployqt-continuous-x86_64.AppImage
sudo mv linuxdeployqt-continuous-x86_64.AppImage /usr/local/bin/linuxdeployqt
```

2. **构建应用程序**
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

3. **创建AppImage**
```bash
# 创建AppImage
linuxdeployqt BrowserSplitScreen -appimage

# 或指定输出目录
linuxdeployqt BrowserSplitScreen -appimage -extra-plugins=platforms,webengine
```

### 方法二：创建DEB包

1. **创建包结构**
```bash
mkdir -p browser-split-screen_1.0.0_amd64/DEBIAN
mkdir -p browser-split-screen_1.0.0_amd64/usr/bin
mkdir -p browser-split-screen_1.0.0_amd64/usr/share/applications
mkdir -p browser-split-screen_1.0.0_amd64/usr/share/icons/hicolor/256x256/apps
```

2. **复制文件**
```bash
# 复制可执行文件
cp BrowserSplitScreen browser-split-screen_1.0.0_amd64/usr/bin/

# 复制桌面文件
cat > browser-split-screen_1.0.0_amd64/usr/share/applications/browser-split-screen.desktop << EOF
[Desktop Entry]
Version=1.0.0
Type=Application
Name=Browser Split Screen
Comment=Multi-window browser tool
Exec=BrowserSplitScreen
Icon=browser-split-screen
Terminal=false
Categories=Network;WebBrowser;
EOF

# 复制图标
cp icon.png browser-split-screen_1.0.0_amd64/usr/share/icons/hicolor/256x256/apps/browser-split-screen.png
```

3. **创建控制文件**
```bash
cat > browser-split-screen_1.0.0_amd64/DEBIAN/control << EOF
Package: browser-split-screen
Version: 1.0.0
Section: web
Priority: optional
Architecture: amd64
Depends: libqt6core6, libqt6gui6, libqt6widgets6, libqt6webenginewidgets6, libsqlite3-0
Maintainer: QunKong Team <support@qunkong.com>
Description: Multi-window browser tool
 A Qt-based multi-window browser tool that supports 1-16 windows
 with user authentication, URL recording, and fullscreen display.
EOF
```

4. **构建DEB包**
```bash
dpkg-deb --build browser-split-screen_1.0.0_amd64
```

### 方法三：创建RPM包

1. **安装rpmbuild**
```bash
# CentOS/RHEL
sudo yum install rpm-build rpmdevtools

# Fedora
sudo dnf install rpm-build rpmdevtools
```

2. **创建RPM构建环境**
```bash
rpmdev-setuptree
```

3. **创建spec文件**
```bash
cat > ~/rpmbuild/SPECS/browser-split-screen.spec << EOF
Name:           browser-split-screen
Version:        1.0.0
Release:        1%{?dist}
Summary:        Multi-window browser tool

License:        MIT
URL:            https://github.com/qunkong/browser-split-screen
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  qt6-qtbase-devel qt6-qtwebengine-devel cmake gcc-c++
Requires:       qt6-qtbase qt6-qtwebengine sqlite

%description
A Qt-based multi-window browser tool that supports 1-16 windows
with user authentication, URL recording, and fullscreen display.

%prep
%setup -q

%build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

%install
mkdir -p %{buildroot}/usr/bin
mkdir -p %{buildroot}/usr/share/applications
mkdir -p %{buildroot}/usr/share/icons/hicolor/256x256/apps

cp build/BrowserSplitScreen %{buildroot}/usr/bin/
cp browser-split-screen.desktop %{buildroot}/usr/share/applications/
cp icon.png %{buildroot}/usr/share/icons/hicolor/256x256/apps/browser-split-screen.png

%files
/usr/bin/BrowserSplitScreen
/usr/share/applications/browser-split-screen.desktop
/usr/share/icons/hicolor/256x256/apps/browser-split-screen.png

%changelog
* $(date '+%a %b %d %Y') QunKong Team <support@qunkong.com> - 1.0.0-1
- Initial release
EOF
```

4. **构建RPM包**
```bash
rpmbuild -ba ~/rpmbuild/SPECS/browser-split-screen.spec
```

## Docker部署

### 创建Dockerfile

```dockerfile
FROM ubuntu:22.04

# 安装依赖
RUN apt-get update && apt-get install -y \
    qt6-base-dev \
    qt6-webengine-dev \
    cmake \
    build-essential \
    libsqlite3-dev \
    xvfb \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /app

# 复制源代码
COPY . .

# 构建应用程序
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build .

# 创建启动脚本
RUN echo '#!/bin/bash\nxvfb-run -a ./build/BrowserSplitScreen' > start.sh && \
    chmod +x start.sh

# 暴露端口（如果需要远程访问）
EXPOSE 8080

# 启动应用程序
CMD ["./start.sh"]
```

### 构建和运行Docker镜像

```bash
# 构建镜像
docker build -t browser-split-screen .

# 运行容器
docker run -it --rm \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    browser-split-screen

# 或使用VNC访问
docker run -it --rm \
    -p 5900:5900 \
    -e VNC_PASSWORD=password \
    browser-split-screen
```

## CI/CD自动化部署

### GitHub Actions配置

创建`.github/workflows/build.yml`：

```yaml
name: Build and Deploy

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup Qt
      uses: jurplel/install-qt-action@v3
      with:
        version: '6.9.1'
        modules: 'qtwebengine'
    
    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release
    
    - name: Build
      run: cmake --build build --config Release
    
    - name: Deploy
      run: |
        cd build
        windeployqt --release --no-translations BrowserSplitScreen.exe
    
    - name: Upload Artifacts
      uses: actions/upload-artifact@v3
      with:
        name: windows-build
        path: build/

  build-macos:
    runs-on: macos-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup Qt
      uses: jurplel/install-qt-action@v3
      with:
        version: '6.9.1'
        modules: 'qtwebengine'
    
    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release
    
    - name: Build
      run: cmake --build build --config Release
    
    - name: Create App Bundle
      run: |
        mkdir -p BrowserSplitScreen.app/Contents/MacOS
        cp build/BrowserSplitScreen BrowserSplitScreen.app/Contents/MacOS/
        # 创建Info.plist等...
    
    - name: Deploy
      run: macdeployqt BrowserSplitScreen.app
    
    - name: Upload Artifacts
      uses: actions/upload-artifact@v3
      with:
        name: macos-build
        path: BrowserSplitScreen.app/

  build-linux:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Install Dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y qt6-base-dev qt6-webengine-dev cmake build-essential libsqlite3-dev
    
    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release
    
    - name: Build
      run: cmake --build build
    
    - name: Create AppImage
      run: |
        wget https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
        chmod +x linuxdeployqt-continuous-x86_64.AppImage
        ./linuxdeployqt-continuous-x86_64.AppImage build/BrowserSplitScreen -appimage
    
    - name: Upload Artifacts
      uses: actions/upload-artifact@v3
      with:
        name: linux-build
        path: build/
```

## 部署检查清单

### 通用检查项
- [ ] 应用程序能正常启动
- [ ] 所有功能正常工作
- [ ] 数据库文件正确创建
- [ ] 用户界面显示正常
- [ ] 网络连接功能正常
- [ ] 文件权限设置正确

### Windows特定检查项
- [ ] 所有DLL文件已包含
- [ ] 注册表项正确设置
- [ ] 卸载程序正常工作
- [ ] 快捷方式正确创建
- [ ] 防火墙规则（如需要）

### macOS特定检查项
- [ ] 应用程序包结构正确
- [ ] 代码签名有效
- [ ] 公证状态（如需要）
- [ ] 权限设置正确
- [ ] 沙盒配置（如需要）

### Linux特定检查项
- [ ] 依赖库已正确链接
- [ ] 桌面文件正确
- [ ] 图标文件存在
- [ ] 包管理器集成
- [ ] 系统服务配置（如需要）

## 性能优化建议

1. **启动时间优化**
   - 延迟加载非关键组件
   - 使用预编译头文件
   - 优化数据库初始化

2. **内存使用优化**
   - 及时释放不用的资源
   - 使用对象池
   - 监控内存泄漏

3. **网络性能优化**
   - 启用HTTP/2支持
   - 使用连接池
   - 实现缓存机制

4. **用户体验优化**
   - 添加启动画面
   - 实现进度指示器
   - 提供错误恢复机制

## 最新功能更新

### 窗口居中功能 (v1.2.0)

**功能描述**: 当用户修改窗口列数量设置时，应用程序会自动调整窗口宽度并居中到屏幕中央。

**技术实现**:
- 新增 `centerWindowOnScreen()` 方法
- 在 `applyWindowColumns()` 和 `loadSubWindowsToLayout()` 中调用居中功能
- 支持所有列数量设置（1列、2列、3列）
- 每次变更列数量时都会重新计算和调整窗口宽度

**使用方法**:
1. 打开应用程序设置
2. 修改"窗口列数量"选项
3. 应用程序窗口会自动居中到屏幕中央

**技术细节**:
```cpp
// 窗口宽度调整和居中功能
void MainWindow::applyWindowColumns(int columns)
{
    // 计算所需窗口宽度
    int requiredWidth = calculateRequiredWindowWidth(columns);
    
    // 设置最小窗口宽度
    setMinimumWidth(requiredWidth);
    
    // 总是调整窗口宽度到所需宽度
    resize(requiredWidth, height());
    
    // 居中窗口到屏幕中央
    centerWindowOnScreen();
}

void MainWindow::centerWindowOnScreen()
{
    // 获取主屏幕
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) return;
    
    // 获取屏幕几何信息
    QRect screenGeometry = screen->geometry();
    
    // 计算居中位置
    QSize windowSize = size();
    int x = screenGeometry.x() + (screenGeometry.width() - windowSize.width()) / 2;
    int y = screenGeometry.y() + (screenGeometry.height() - windowSize.height()) / 2;
    
    // 移动窗口到居中位置
    move(x, y);
}
```

**支持的列数量设置**:
- **1列**: 子窗口宽度880像素，高度528像素（5:3比例），应用程序宽度仅显示一个窗口宽度
- **2列**: 子窗口宽度500像素，高度300像素（5:3比例），应用程序宽度显示两个窗口宽度  
- **3列**: 子窗口宽度500像素，高度300像素（5:3比例），应用程序宽度显示三个窗口宽度

**5:3比例计算**:
- 高度 = 宽度 × 3/5
- 1列: 880 × 3/5 = 528像素
- 2列/3列: 500 × 3/5 = 300像素

---

**注意**: 部署前请确保在目标环境中充分测试所有功能，并根据实际需求调整配置参数。

