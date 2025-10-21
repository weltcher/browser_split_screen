# Cookie隔离功能修复说明

## 问题描述
子窗口加载时使用公用的同一份cookie，而不是按照子窗口ID独立存储和加载cookie。

## 修复内容

### 1. 修复构造函数中的cookie加载问题
**文件**: `src/BrowserWidget.cpp`
**问题**: 在构造函数中调用`loadState()`时，`m_subWindowId`还是-1，导致cookie加载失败
**修复**: 将构造函数中的`loadState()`改为只调用`loadWindowState()`，避免在`subWindowId`未设置时加载cookie

### 2. 改进setSubWindowId方法
**文件**: `src/BrowserWidget.cpp`
**修复**: 在`setSubWindowId()`方法中添加自动加载cookie的逻辑，当设置子窗口ID时自动加载对应的cookie

### 3. 优化MainWindow中的子窗口加载
**文件**: `src/MainWindow.cpp`
**修复**: 移除重复的`loadCookies()`调用，因为`setSubWindowId()`现在会自动加载cookie

### 4. 改进cookie加载时机
**文件**: `src/BrowserWidget.cpp`
**修复**: 
- 在`loadCookies()`中检查页面是否正在加载，避免在页面未准备好时执行cookie脚本
- 在`onLoadFinished()`中确保只在`subWindowId`有效时保存cookie

### 5. 添加调试信息
**文件**: `src/BrowserWidget.cpp`
**修复**: 在关键方法中添加调试输出，帮助诊断cookie加载和保存问题

## Cookie文件存储结构

```
~/.browser_split_screen/cookies/
├── cookies_1.json    # 子窗口1的cookie
├── cookies_2.json    # 子窗口2的cookie
├── cookies_3.json    # 子窗口3的cookie
└── ...
```

每个子窗口都有独立的cookie文件，文件名格式为`cookies_{subWindowId}.json`。

## 测试方法

1. 运行应用程序
2. 创建多个子窗口
3. 在不同子窗口中访问不同的网站并登录
4. 重启应用程序
5. 检查每个子窗口是否保持了独立的登录状态
6. 运行测试脚本验证cookie隔离：

```bash
./test_cookie_isolation.sh
```

## 预期结果

- 每个子窗口都有独立的cookie文件
- 不同子窗口的cookie不会互相影响
- 重启应用程序后，每个子窗口都能保持独立的登录状态
- Cookie文件内容不重复，证明隔离功能正常工作

## 技术细节

### Cookie保存流程
1. 页面加载完成后，`onLoadFinished()`被调用
2. 检查`m_subWindowId > 0`，如果有效则保存cookie
3. 使用JavaScript获取当前页面的所有cookie
4. 将cookie数据保存到`cookies_{subWindowId}.json`文件

### Cookie加载流程
1. 调用`setSubWindowId()`设置子窗口ID
2. 自动调用`loadCookies()`加载对应子窗口的cookie
3. 解析cookie文件并生成JavaScript脚本
4. 在页面加载完成后执行cookie脚本，恢复登录状态

### 关键改进点
- **时序控制**: 确保在正确的时机加载和保存cookie
- **ID验证**: 只有在`subWindowId`有效时才进行cookie操作
- **文件隔离**: 每个子窗口使用独立的cookie文件
- **调试支持**: 添加详细的调试信息帮助问题诊断
