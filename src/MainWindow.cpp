#include "MainWindow.h"
#include <QApplication>
#include <QScreen>
#include <QShortcut>
#include <QInputDialog>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDialog>
#include <QGroupBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QShowEvent>
#include <QLineEdit> // Added for password fields

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_windowManager(nullptr)
    , m_fullscreenBrowser(nullptr)
    , m_fullscreenWidgetPosition(-1)
    , m_loginDialog(nullptr)
    , m_subWindowManager(nullptr)
    , m_isLoggedIn(false)
    , m_currentLayout(0)
    , m_autoSaveTimer(new QTimer(this))
    , m_initialized(false)
    , m_loadingSubwindows(false) // Added for loading guard
{
    // Create toolbar actions here (independent of menu)
    m_refreshAllAction = new QAction("刷新全部(&R)", this);
    m_refreshAllAction->setShortcut(QKeySequence::Refresh);
    m_refreshAllAction->setIcon(QIcon(":/icons/refresh.png"));
    
    m_subWindowManagerAction = new QAction("子窗口管理(&W)", this);
    m_subWindowManagerAction->setShortcut(QKeySequence("Ctrl+Shift+W"));
    m_subWindowManagerAction->setIcon(QIcon(":/icons/window.png"));
    
    m_settingsAction = new QAction("设置(&S)", this);
    m_settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    m_settingsAction->setIcon(QIcon(":/icons/settings.png"));
    
    m_accountManagerAction = new QAction("账户管理(&M)", this);
    m_accountManagerAction->setShortcut(QKeySequence("Ctrl+Shift+A"));
    m_accountManagerAction->setIcon(QIcon(":/icons/account.png"));
    
    setupUI();
    setupConnections();
    setupShortcuts();
    
    // Load settings
    loadSettings();
    
    // Check login
    bool hasValidSession = checkLogin();
    if (hasValidSession) {
        // Load user from session
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        QString username;
        bool remember;
        if (dbManager->loadUserSession(username, remember)) {
            m_currentUser = username;
            m_isLoggedIn = true;
            
            
            // Update status bar
            QLabel* userLabel = qobject_cast<QLabel*>(m_statusBar->children().last());
            if (userLabel) {
                userLabel->setText(QString("用户: %1").arg(m_currentUser));
            }
            
            // Set window title
            setWindowTitle(QString("Browser Split Screen - %1").arg(m_currentUser));
            
            // Show main interface but defer loading subwindows
            showMainInterface();
        } else {
            showLoginDialog();
        }
    } else {
        showLoginDialog();
    }
    
    // Setup auto-save timer (save every 2 minutes)
    m_autoSaveTimer->setInterval(120000);
    m_autoSaveTimer->setSingleShot(false);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::onAutoSave);
    m_autoSaveTimer->start();
    
    // Update status bar every 5 seconds
    QTimer* statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    statusTimer->start(5000);
    updateStatusBar();
}

MainWindow::~MainWindow()
{
    saveSettings();
    if (m_windowManager) {
        m_windowManager->saveAllStates();
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("Browser Split Screen - 浏览器分屏工具");
    setMinimumSize(800, 600);
    resize(1200, 800);
    
    // Center window on screen
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
    
    // setupMenuBar();  // Commented out: Remove menu bar as per request
    
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
}

void MainWindow::setupMenuBar()
{
    m_menuBar = menuBar();
    
    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("文件(&F)");
    
    // m_logoutAction = new QAction("退出登�?&L)", this);
    // m_logoutAction->setShortcut(QKeySequence("Ctrl+Shift+L"));
    // m_logoutAction->setIcon(QIcon(":/icons/logout.png"));
    // fileMenu->addAction(m_logoutAction);
    
    fileMenu->addSeparator();
    
    // m_exitAction = new QAction("退�?&X)", this);
    // m_exitAction->setShortcut(QKeySequence::Quit);
    // m_exitAction->setIcon(QIcon(":/icons/exit.png"));
    // fileMenu->addAction(m_exitAction);
    
    // Edit menu
    QMenu* editMenu = m_menuBar->addMenu("编辑(&E)");
    
    // m_refreshAllAction = new QAction("刷新全部(&R)", this);
    // m_refreshAllAction->setShortcut(QKeySequence::Refresh);
    // m_refreshAllAction->setIcon(QIcon(":/icons/refresh.png"));
    // editMenu->addAction(m_refreshAllAction);
    
    // 移除主页全部，改为账户管理菜单
    
    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("视图(&V)");
    
    // 布局现在由子窗口数据自动决定，不需要手动选择
    
    // Tools menu
    QMenu* toolsMenu = m_menuBar->addMenu("工具(&T)");
    
    
    // m_settingsAction = new QAction("设置(&S)", this);
    // m_settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    // m_settingsAction->setIcon(QIcon(":/icons/settings.png"));
    // toolsMenu->addAction(m_settingsAction);
    
    // Help menu
    QMenu* helpMenu = m_menuBar->addMenu("帮助(&H)");
    
    // m_aboutAction = new QAction("关于(&A)", this);
    // m_aboutAction->setIcon(QIcon(":/icons/about.png"));
    // helpMenu->addAction(m_aboutAction);
    
    // Account menu (放在最右)
    QMenu* accountMenu = m_menuBar->addMenu("账户(&A)");
    
    // m_accountManagerAction = new QAction("账户管理(&M)", this);
    // m_accountManagerAction->setShortcut(QKeySequence("Ctrl+Shift+A"));
    // m_accountManagerAction->setIcon(QIcon(":/icons/account.png"));
    // accountMenu->addAction(m_accountManagerAction);
}

void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar("主工具栏");
    m_toolBar->setMovable(false);
    
    // Add toolbar actions (menu removed, so only these)
    m_toolBar->addAction(m_refreshAllAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_subWindowManagerAction);
    m_toolBar->addAction(m_settingsAction);
    m_toolBar->addAction(m_accountManagerAction);
}

void MainWindow::setupStatusBar()
{
    m_statusBar = statusBar();
    
    m_statusLabel = new QLabel("就绪");
    m_statusBar->addWidget(m_statusLabel);
    
    m_globalProgressBar = new QProgressBar();
    m_globalProgressBar->setVisible(false);
    m_globalProgressBar->setMaximumWidth(200);
    m_statusBar->addPermanentWidget(m_globalProgressBar);
    
    QLabel* userLabel = new QLabel("用户: 未登录");
    m_statusBar->addPermanentWidget(userLabel);
}

void MainWindow::setupCentralWidget()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_centralLayout = new QVBoxLayout(m_centralWidget);
    m_centralLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create stacked widget for main view, logout view and fullscreen view
    m_stackedWidget = new QStackedWidget(this);
    m_centralLayout->addWidget(m_stackedWidget);
    
    // Main widget (with browser windows)
    m_mainWidget = new QWidget();
    m_stackedWidget->addWidget(m_mainWidget);
    
    // Empty state label
    m_emptyStateLabel = new QLabel("暂无子窗口\n请点击工具栏中的\"子窗口管理\"按钮添加子窗口", m_mainWidget);
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->setStyleSheet("QLabel { color: gray; font-size: 16px; padding: 50px; }");
    m_emptyStateLabel->setWordWrap(true);
    
    // Logout widget (shown when user is logged out)
    m_logoutWidget = new QWidget();
    m_stackedWidget->addWidget(m_logoutWidget);
    setupLogoutWidget();
    
    // Fullscreen widget (placeholder)
    m_fullscreenWidget = new QWidget();
    m_stackedWidget->addWidget(m_fullscreenWidget);
    
    // Initialize window manager
    m_windowManager = new WindowManager(m_mainWidget, this);
}

void MainWindow::setupLogoutWidget()
{
    QVBoxLayout* logoutLayout = new QVBoxLayout(m_logoutWidget);
    logoutLayout->setContentsMargins(50, 50, 50, 50);
    logoutLayout->setSpacing(30);
    
    // Main container with centered content
    QWidget* container = new QWidget();
    container->setMaximumWidth(400);
    container->setMinimumHeight(300);
    logoutLayout->addWidget(container, 0, Qt::AlignCenter);
    
    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(20);
    
    // App icon/logo (using a large label as placeholder)
    QLabel* logoLabel = new QLabel("🔐");
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setStyleSheet("QLabel { font-size: 64px; color: #666; }");
    containerLayout->addWidget(logoLabel);
    
    // Title
    QLabel* titleLabel = new QLabel("未登录");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 24px; font-weight: bold; color: #333; margin: 10px; }");
    containerLayout->addWidget(titleLabel);
    
    // Description
    QLabel* descLabel = new QLabel("您已退出登录，请重新登录以继续使用");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; margin: 10px; }");
    containerLayout->addWidget(descLabel);
    
    // Login button
    m_logoutLoginButton = new QPushButton("重新登录");
    m_logoutLoginButton->setMinimumHeight(40);
    m_logoutLoginButton->setStyleSheet(
        "QPushButton { "
        "background-color: #007bff; "
        "color: white; "
        "border: none; "
        "padding: 10px 20px; "
        "border-radius: 5px; "
        "font-size: 16px; "
        "font-weight: bold; "
        "}"
        "QPushButton:hover { "
        "background-color: #0056b3; "
        "}"
        "QPushButton:pressed { "
        "background-color: #004085; "
        "}"
    );
    containerLayout->addWidget(m_logoutLoginButton);
    
    // Connect login button
    connect(m_logoutLoginButton, &QPushButton::clicked, this, &MainWindow::showLoginDialog);
    
    // Add some spacing at the bottom
    containerLayout->addStretch();
}

void MainWindow::setupConnections()
{
    // Toolbar actions only
    connect(m_refreshAllAction, &QAction::triggered, this, &MainWindow::onRefreshAll);
    connect(m_subWindowManagerAction, &QAction::triggered, this, &MainWindow::onSubWindowManager);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onSettings);
    connect(m_accountManagerAction, &QAction::triggered, this, &MainWindow::onAccountManager);
    
    // Window manager signals (unchanged)
    connect(m_windowManager, &WindowManager::fullscreenRequested, 
            this, &MainWindow::onFullscreenRequested);
    connect(m_windowManager, &WindowManager::allWidgetsCreated, this, &MainWindow::onAllWidgetsCreated);
}

void MainWindow::setupShortcuts()
{
    // Global shortcuts
    new QShortcut(QKeySequence("F11"), this, [this]() {
        if (m_fullscreenBrowser) {
            hideFullscreenWindow();
        }
    });
    
    new QShortcut(QKeySequence("Escape"), this, [this]() {
        if (m_fullscreenBrowser) {
            hideFullscreenWindow();
        }
    });
}

bool MainWindow::checkLogin()
{
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    QString username;
    bool remember;
    
    return dbManager->loadUserSession(username, remember);
}

void MainWindow::showLoginDialog()
{
    m_loginDialog = new LoginDialog(this);
    connect(m_loginDialog, &LoginDialog::accepted, this, &MainWindow::handleLogin);
    connect(m_loginDialog, &LoginDialog::rejected, this, &MainWindow::close);
    m_loginDialog->show();
}

void MainWindow::handleLogin()
{
    
    if (m_loginDialog && m_loginDialog->isLoginSuccessful()) {
        // Login through dialog
        m_currentUser = m_loginDialog->getUsername();
        m_isLoggedIn = true;
        
        
        // Update status bar
        QLabel* userLabel = qobject_cast<QLabel*>(m_statusBar->children().last());
        if (userLabel) {
            userLabel->setText(QString("用户: %1").arg(m_currentUser));
        }
        
        // Set window title
        setWindowTitle(QString("Browser Split Screen - %1").arg(m_currentUser));
        
        // FIXED: Explicitly load subwindows after login to initialize windowManager
        loadSubWindowsToLayout();
        m_initialized = true;  // FIXED: Set to true to prevent duplicate loading in showEvent
        
        // Show main interface
        showMainInterface();
        
        m_loginDialog->deleteLater();
        m_loginDialog = nullptr;
    } else if (!m_loginDialog) {
        close();
    } else {
        qDebug() << "Login failed, closing application";
        close();
    }
}

void MainWindow::handleLogout()
{
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    dbManager->clearUserSession();
    
    m_isLoggedIn = false;
    m_currentUser.clear();
    
    showLoginDialog();
}

void MainWindow::saveSettings()
{
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    if (!dbManager) {
        qWarning() << "MainWindow::saveSettings: DatabaseManager instance unavailable";
        return;
    }

    dbManager->setAppSetting("geometry", saveGeometry());
    dbManager->setAppSetting("windowState", saveState());
    dbManager->setAppSetting("currentLayout", m_currentLayout);
    dbManager->setAppSetting("currentUser", m_currentUser);
}

void MainWindow::loadSettings()
{
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    if (!dbManager) {
        qWarning() << "MainWindow::loadSettings: DatabaseManager instance unavailable";
        return;
    }

    QVariant geometry = dbManager->getAppSetting("geometry");
    if (geometry.isValid()) {
        restoreGeometry(geometry.toByteArray());
    }

    QVariant windowState = dbManager->getAppSetting("windowState");
    if (windowState.isValid()) {
        restoreState(windowState.toByteArray());
    }

    m_currentLayout = dbManager->getAppSetting("currentLayout", 0).toInt();
    m_currentUser = dbManager->getAppSetting("currentUser").toString();
    
    // Load window count setting (default to 4)
    int windowCount = dbManager->getAppSetting("windowCount", 4).toInt();
    dbManager->setAppSetting("windowCount", windowCount); // Ensure it's saved
    
    // 布局现在由子窗口数据自动决定，不需要手动设置
}

void MainWindow::showFullscreenWindow(BrowserWidget* widget)
{
    
    if (!widget) {
        qDebug() << "MainWindow::showFullscreenWindow: Widget is null, returning";
        return;
    }
    
    m_fullscreenBrowser = widget;
    
    // Find the position of this widget in the layout
    QList<BrowserWidget*> widgets = m_windowManager->getBrowserWidgets();
    m_fullscreenWidgetPosition = widgets.indexOf(widget);
    
    // Detach widget from the main layout
    m_windowManager->detachWidgetFromLayout(widget);
    
    // Clear any existing layout
    QLayout* existingLayout = m_fullscreenWidget->layout();
    if (existingLayout) {
        QLayoutItem* item;
        while ((item = existingLayout->takeAt(0)) != nullptr) {
            // Don't delete the widget, just remove it from layout
            if (item->widget()) {
                qDebug() << "MainWindow::showFullscreenWindow: Setting widget parent to null";
                item->widget()->setParent(nullptr);
            }
            delete item;
        }
        delete existingLayout;
    } else {
    }
    
    // Create fullscreen layout
    QVBoxLayout* fullscreenLayout = new QVBoxLayout(m_fullscreenWidget);
    fullscreenLayout->setContentsMargins(0, 0, 0, 0);
    fullscreenLayout->setSpacing(0);
    
    // Add widget to fullscreen layout
    // Note: We don't change the parent to avoid WebView recreation
    // The widget will be added to the layout without changing its parent
    fullscreenLayout->addWidget(widget);
    
    // Switch to fullscreen view
    m_stackedWidget->setCurrentWidget(m_fullscreenWidget);
    
    // Save cookies before entering fullscreen to ensure state is preserved
    if (widget->getSubWindowId() > 0) {
        widget->saveCookies();
    }
    
    // Set fullscreen mode
    widget->setFullscreenMode(true);
    
    // Hide menu bar and tool bar
    // m_menuBar->hide(); // Removed
    m_toolBar->hide();
    m_statusBar->hide();
    
    // Set window to fullscreen
    showFullScreen();
    
    // Force the widget to resize to fill the entire screen
    QTimer::singleShot(100, [this, widget]() {
        if (widget && m_fullscreenBrowser == widget) {
            // Get the available screen geometry
            QScreen* screen = QApplication::primaryScreen();
            if (screen) {
                QRect screenGeometry = screen->geometry();
                widget->setGeometry(0, 0, screenGeometry.width(), screenGeometry.height());
                widget->setMinimumSize(screenGeometry.size());
                widget->setMaximumSize(screenGeometry.size());
            } else {
            }
        } else {
        }
    });
}

void MainWindow::hideFullscreenWindow()
{
    if (!m_fullscreenBrowser) return;
    
    // Exit fullscreen mode
    showNormal();
    
    // Show menu bar and tool bar
    // m_menuBar->show(); // Removed
    m_toolBar->show();
    m_statusBar->show();
    
    // Set widget back to normal mode
    m_fullscreenBrowser->setFullscreenMode(false);
    
    // Reload cookies after exiting fullscreen to ensure login state is preserved
    if (m_fullscreenBrowser->getSubWindowId() > 0) {
        m_fullscreenBrowser->loadCookies();
    }
    
    // Return widget to main view
    // Note: We don't change the parent here to avoid WebView recreation
    // The widget will be re-attached to the layout without changing its parent
    m_windowManager->setParentWidget(m_mainWidget);
    
    // Re-attach widget to its original position in the layout
    if (m_fullscreenWidgetPosition >= 0) {
        m_windowManager->attachWidgetToLayout(m_fullscreenBrowser, m_fullscreenWidgetPosition);
    }
    
    // Force layout update to ensure proper sizing
    m_windowManager->forceLayoutUpdate();
    
    // Ensure all widgets have consistent widths after exiting fullscreen
    m_windowManager->synchronizeWidgetWidths();
    
    // Reset the widget's size constraints after exiting fullscreen
    QTimer::singleShot(50, [this]() {
        if (m_fullscreenBrowser) {
            m_fullscreenBrowser->setMinimumSize(300, 200);
            m_fullscreenBrowser->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            m_fullscreenBrowser->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        }
    });
    
    // Switch back to main view
    m_stackedWidget->setCurrentWidget(m_mainWidget);
    
    m_fullscreenBrowser = nullptr;
    m_fullscreenWidgetPosition = -1;
}


void MainWindow::onRefreshAll()
{
    QList<BrowserWidget*> widgets = m_windowManager->getBrowserWidgets();
    for (BrowserWidget* widget : widgets) {
        widget->refresh();
    }
}




void MainWindow::onSubWindowManager()
{
    if (!m_subWindowManager) {
        m_subWindowManager = new SubWindowManager(this);
        connect(m_subWindowManager, &SubWindowManager::subWindowAdded, 
                this, &MainWindow::onSubWindowAdded);
        connect(m_subWindowManager, &SubWindowManager::subWindowRefreshRequested, 
                this, &MainWindow::onNewSubWindowRefresh);
        connect(m_subWindowManager, &SubWindowManager::subWindowUpdated, 
                this, &MainWindow::onSubWindowUpdated);
        connect(m_subWindowManager, &SubWindowManager::subWindowDeleted, 
                this, &MainWindow::onSubWindowDeleted);
    }
    
    m_subWindowManager->show();
    m_subWindowManager->raise();
    m_subWindowManager->activateWindow();
}

void MainWindow::onSettings()
{
    // Create settings dialog
    QDialog settingsDialog(this);
    settingsDialog.setWindowTitle("设置");
    settingsDialog.setModal(true);
    settingsDialog.resize(400, 200);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&settingsDialog);
    
    // Window column count setting
    QGroupBox* windowGroup = new QGroupBox("窗口列数", &settingsDialog);
    QVBoxLayout* windowLayout = new QVBoxLayout(windowGroup);
    
    QLabel* windowLabel = new QLabel("选择窗口列数", windowGroup);
    windowLayout->addWidget(windowLabel);
    
    QComboBox* windowCountCombo = new QComboBox(windowGroup);
    windowCountCombo->addItems({"1列", "2列", "3列"});
    
    // Set current value
    DatabaseManager* dbSettings = DatabaseManager::getInstance();
    int currentColumns = dbSettings ? dbSettings->getAppSetting("windowColumns", 2).toInt() : 2; // Default to 2 columns
    int index = currentColumns - 1; // Convert to 0-based index
    if (index >= 0 && index < 3) {
        windowCountCombo->setCurrentIndex(index);
    }
    
    windowLayout->addWidget(windowCountCombo);
    mainLayout->addWidget(windowGroup);
    
    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(&settingsDialog);
    QPushButton* okButton = buttonBox->addButton("确认", QDialogButtonBox::AcceptRole);
    QPushButton* cancelButton = buttonBox->addButton("取消", QDialogButtonBox::RejectRole);
    mainLayout->addWidget(buttonBox);
    
    connect(okButton, &QPushButton::clicked, &settingsDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &settingsDialog, &QDialog::reject);
    
    if (settingsDialog.exec() == QDialog::Accepted) {
        int newColumns = windowCountCombo->currentIndex() + 1; // Convert from 0-based to 1-based
        if (dbSettings) {
            dbSettings->setAppSetting("windowColumns", newColumns);
        }
        
        // Apply the new column count
        applyWindowColumns(newColumns);
        
        // QMessageBox::information(this, "设置", QString("窗口列数量已设置为 %1 列").arg(newColumns));
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "关于", 
                      "Browser Split Screen v1.0.0\n\n"
                      "一个基于Qt的多窗口浏览器工具\n"
                      "支持1-16个窗口同时浏览\n\n"
                      "开发: QunKong Team\n"
                      "技术栈: Qt 6.9.1 + SQLite3");
}


void MainWindow::onExit()
{
    close();
}

void MainWindow::onFullscreenRequested(BrowserWidget* widget)
{
    
    if (m_fullscreenBrowser) {
        // Already in fullscreen mode, exit fullscreen
        hideFullscreenWindow();
    } else {
        // Not in fullscreen mode, enter fullscreen
        showFullscreenWindow(widget);
    }
}

void MainWindow::onWindowCloseRequested()
{
    // This is handled by WindowManager
}

void MainWindow::updateStatusBar()
{
    if (m_windowManager) {
        int windowCount = m_windowManager->getCurrentWindowCount();
        m_statusLabel->setText(QString("当前布局: %1 窗口").arg(windowCount));
    }
}

void MainWindow::onAutoSave()
{
    if (m_windowManager) {
        m_windowManager->saveAllStates();
    }
    saveSettings();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    if (m_windowManager) {
        m_windowManager->saveAllStates();
    }
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F11) {
        if (m_fullscreenBrowser) {
            hideFullscreenWindow();
        }
        event->accept();
    } else if (event->key() == Qt::Key_Escape && m_fullscreenBrowser) {
        hideFullscreenWindow();
        event->accept();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::onSubWindowAdded(const QJsonObject& subWindow)
{
    int newSubId = subWindow["id"].toInt();
    QString newName = subWindow["name"].toString();
    QString newUrl = subWindow["url"].toString();

    if (!m_windowManager) {
        qWarning() << "MainWindow::onSubWindowAdded: m_windowManager is null, cannot add";
        return;
    }

    // Reload all subwindows from database to ensure proper assignment
    loadSubWindowsToLayout();

}

void MainWindow::onSubWindowUpdated(const QJsonObject& subWindow)
{
    if (!m_windowManager) return;

    int subId = subWindow["id"].toInt();
    QString newUrl = subWindow["url"].toString();
    QString newName = subWindow["name"].toString();


    BrowserWidget* targetWidget = m_windowManager->findWidgetBySubId(subId);
    if (targetWidget) {
        targetWidget->setSubWindowName(newName);
        if (!newUrl.isEmpty() && targetWidget->getCurrentUrl() != newUrl) {
            targetWidget->loadUrl(newUrl);
        } else {
        }

        // Update window_configs with new URL from sub_windows (use subId as window_id)
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        if (dbManager) {
            QJsonObject geometry;
            geometry["x"] = targetWidget->x();
            geometry["y"] = targetWidget->y();
            geometry["width"] = targetWidget->width();
            geometry["height"] = targetWidget->height();

            if (dbManager->saveWindowConfig(subId, subId, newUrl, newName, geometry)) {
            } else {
                qWarning() << "Failed to update window_config for subId:" << subId;
            }
        }
    } else {
        loadSubWindowsToLayout();
    }
}

void MainWindow::onSubWindowDeleted(int subWindowId)
{
    
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    if (dbManager) {
        dbManager->deleteWindowConfigsBySubId(subWindowId);
    }
    
    // Full reload to remove and re-layout
    loadSubWindowsToLayout();
}

void MainWindow::onAllWidgetsCreated()
{
    if (m_loadingSubwindows) {
        return;
    }
    loadSubWindowsToLayout();
}

void MainWindow::loadSubWindowsToLayout()
{
    
    if (m_loadingSubwindows) {
        return;
    }
    m_loadingSubwindows = true;
    
    if (!m_windowManager) {
        qDebug() << "ERROR: m_windowManager is null!";
        m_loadingSubwindows = false;
        return;
    }
    
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    if (!dbManager) {
        qWarning() << "loadSubWindowsToLayout: DatabaseManager is null!";
        m_loadingSubwindows = false;
        return;
    }
    QList<QJsonObject> subWindows = dbManager->getAllSubWindows();
    
    for (int i = 0; i < subWindows.size(); ++i) {
        QJsonObject subWindow = subWindows[i];
    }
    
    int columnCount = dbManager->getAppSetting("windowColumns", 2).toInt();
    int windowCount = subWindows.size();
    
    
    if (windowCount == 0) {
        m_windowManager->setLayout(0);
        m_currentLayout = 0;
        
        if (m_emptyStateLabel) {
            m_emptyStateLabel->show();
        }
        
        updateStatusBar();
        m_loadingSubwindows = false;
        return;
    }
    
    // Hide empty state label when there are sub windows
    if (m_emptyStateLabel) {
        m_emptyStateLabel->hide();
    }
    
    m_windowManager->setColumnCount(columnCount);
    m_windowManager->setLayout(windowCount);
    m_currentLayout = windowCount;
    
    // Apply window width based on column count
    int requiredWidth = calculateRequiredWindowWidth(columnCount);
    setMinimumWidth(requiredWidth);
    resize(requiredWidth, height());
    centerWindowOnScreen();
    
    // Update each widget by index order (match subWindows to first N widgets 1:1)
    QList<BrowserWidget*> widgets = m_windowManager->getBrowserWidgets();

    // Assign subwindows to the first N widgets (where N = subWindows.size())
    // Don't rely on isVisible() as widgets may not be visible yet during layout updates
    for (int i = 0; i < subWindows.size() && i < widgets.size(); i++) {
        BrowserWidget* widget = widgets[i];
        if (!widget) {
            qWarning() << "Widget at index" << i << "is null, skipping";
            continue;
        }

        const QJsonObject& subWindow = subWindows[i];
        int subId = subWindow["id"].toInt();
        QString name = subWindow["name"].toString();
        QString url = subWindow["url"].toString();
        int widgetId = widget->getWindowId();


        widget->setSubWindowId(subId);
        widget->setSubWindowName(name);
        if (widget->getCurrentUrl() != url) {
            widget->loadUrl(url);
        } else {
        }
    }

    
    // Force UI update
    m_mainWidget->update();
    m_mainWidget->repaint();
    
    updateStatusBar();
    m_initialized = true;
    m_loadingSubwindows = false;
}

void MainWindow::applyWindowCount(int windowCount)
{
    if (!m_windowManager) return;
    
    // Set the layout to the specified window count
    m_windowManager->setLayout(windowCount);
    m_currentLayout = windowCount;
    
    // Reload sub windows to fit the new layout
    loadSubWindowsToLayout();
}

void MainWindow::applyWindowColumns(int columns)
{
    if (!m_windowManager) return;
    
    
    m_windowManager->setColumnCount(columns);
    
    int requiredWidth = calculateRequiredWindowWidth(columns);
    setMinimumWidth(requiredWidth);
    resize(requiredWidth, height());
    centerWindowOnScreen();
    
    // Update layout without full reload - just resize and manage visibility
    m_windowManager->forceLayoutUpdate();
    DatabaseManager* dbSettings2 = DatabaseManager::getInstance();
    if (dbSettings2) {
        dbSettings2->setAppSetting("windowColumns", columns);
    }
}

int MainWindow::calculateRequiredWindowWidth(int columns)
{
    // Calculate required window width based on column count
    int widgetWidth = (columns == 1) ? 880 : 500;
    int spacing = 5; // Spacing between widgets
    int margins = 20; // Window margins
    
    int requiredWidth = (widgetWidth * columns) + (spacing * (columns - 1)) + margins;

    return requiredWidth;
}

void MainWindow::centerWindowOnScreen()
{
    // Get the primary screen
    QScreen* screen = QApplication::primaryScreen();
    if (!screen) {
        return;
    }
    
    // Get screen geometry
    QRect screenGeometry = screen->geometry();
    
    // Get current window size
    QSize windowSize = size();
    
    // Calculate center position
    int x = screenGeometry.x() + (screenGeometry.width() - windowSize.width()) / 2;
    int y = screenGeometry.y() + (screenGeometry.height() - windowSize.height()) / 2;
    
    
    // Move window to center
    move(x, y);
}

void MainWindow::onLogout()
{
    
    // Show confirmation dialog
    int ret = QMessageBox::question(this, "确认退出登录", 
                                   "确定要退出登录吗？\n这将清除所有子窗口的登录状态。",
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // Clear all browser widgets' login states
    QList<BrowserWidget*> widgets = m_windowManager->getBrowserWidgets();
    for (BrowserWidget* widget : widgets) {
        if (widget && widget->getSubWindowId() > 0) {
            widget->clearLoginState();
        }
    }
    
    // Clear application login state
    m_isLoggedIn = false;
    m_currentUser.clear();
    
    // Update status bar
    QLabel* userLabel = qobject_cast<QLabel*>(m_statusBar->children().last());
    if (userLabel) {
        userLabel->setText("用户: 未登录");
    }
    
    // Set window title
    setWindowTitle("Browser Split Screen - 浏览器分屏工具");
    
    // Show logout page instead of login dialog
    showLogoutPage();
    
}

void MainWindow::showLogoutPage()
{
    // Switch to logout widget
    m_stackedWidget->setCurrentWidget(m_logoutWidget);
    
    // Hide menu bar and toolbar when logged out
    // m_menuBar->hide(); // Removed
    m_toolBar->hide();
    
    // Update status bar to show logout state
    m_statusLabel->setText("未登录状态");
}

void MainWindow::showMainInterface()
{
    
    // Switch to main widget
    m_stackedWidget->setCurrentWidget(m_mainWidget);
    
    // Show menu bar and toolbar when logged in
    // m_menuBar->show(); // Removed
    m_toolBar->show();
    
    // Update status bar
    updateStatusBar();
    
    // FIXED: Log if subwindows already loaded or need loading
    if (m_initialized) {
    } else {
    }
}

void MainWindow::onAccountManager()
{
    
    // Create account management dialog
    QDialog* accountDialog = new QDialog(this);
    accountDialog->setWindowTitle("账户管理");
    accountDialog->setModal(true);
    accountDialog->resize(400, 400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(accountDialog);
    
    // Account info section
    QGroupBox* infoGroup = new QGroupBox("账户信息", accountDialog);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    
    QLabel* loginStatusLabel = new QLabel("登录状态: 已登录", accountDialog);
    loginStatusLabel->setStyleSheet("color: green; font-weight: bold;");
    infoLayout->addWidget(loginStatusLabel);
    
    QLabel* sessionLabel = new QLabel("会话信息: 活跃", accountDialog);
    infoLayout->addWidget(sessionLabel);
    
    mainLayout->addWidget(infoGroup);
    
    // Actions section
    QGroupBox* actionsGroup = new QGroupBox("账户操作", accountDialog);
    QVBoxLayout* actionsLayout = new QVBoxLayout(actionsGroup);
    
    QPushButton* logoutButton = new QPushButton("退出登录", accountDialog);
    logoutButton->setStyleSheet(
        "QPushButton { "
        "background-color: #dc3545; "
        "color: white; "
        "border: none; "
        "padding: 10px; "
        "border-radius: 5px; "
        "font-weight: bold; "
        "}"
        "QPushButton:hover { "
        "background-color: #c82333; "
        "}"
    );
    logoutButton->setMinimumHeight(40);
    
    QPushButton* changePasswordButton = new QPushButton("修改密码", accountDialog);
    changePasswordButton->setStyleSheet(
        "QPushButton { "
        "background-color: #007bff; "
        "color: white; "
        "border: none; "
        "padding: 10px; "
        "border-radius: 5px; "
        "font-weight: bold; "
        "}"
        "QPushButton:hover { "
        "background-color: #0056b3; "
        "}"
    );
    changePasswordButton->setMinimumHeight(40);
    
    actionsLayout->addWidget(logoutButton);
    actionsLayout->addWidget(changePasswordButton);
    
    mainLayout->addWidget(actionsGroup);
    
    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, accountDialog);
    mainLayout->addWidget(buttonBox);
    
    // Connect signals
    connect(logoutButton, &QPushButton::clicked, [this, accountDialog]() {
        accountDialog->accept();
        onLogout();
    });
    
    connect(changePasswordButton, &QPushButton::clicked, [this, accountDialog]() {
        accountDialog->accept();
        
        // Create change password dialog
        QDialog* passwordDialog = new QDialog(this);
        passwordDialog->setWindowTitle("修改密码");
        passwordDialog->setModal(true);
        passwordDialog->resize(350, 250);
        
        QVBoxLayout* pwLayout = new QVBoxLayout(passwordDialog);
        
        // Old password
        QLabel* oldLabel = new QLabel("原密码", passwordDialog);
        QLineEdit* oldEdit = new QLineEdit(passwordDialog);
        oldEdit->setEchoMode(QLineEdit::Password);
        oldEdit->setPlaceholderText("请输入原密码");
        
        // New password
        QLabel* newLabel = new QLabel("新密码", passwordDialog);
        QLineEdit* newEdit = new QLineEdit(passwordDialog);
        newEdit->setEchoMode(QLineEdit::Password);
        newEdit->setPlaceholderText("请输入新密码 (至少6位)");
        
        // Confirm new password
        QLabel* confirmLabel = new QLabel("确认新密码", passwordDialog);
        QLineEdit* confirmEdit = new QLineEdit(passwordDialog);
        confirmEdit->setEchoMode(QLineEdit::Password);
        confirmEdit->setPlaceholderText("请再次输入新密码");
        
        // Error label
        QLabel* errorLabel = new QLabel("", passwordDialog);
        errorLabel->setStyleSheet("color: red; font-size: 12px;");
        
        pwLayout->addWidget(oldLabel);
        pwLayout->addWidget(oldEdit);
        pwLayout->addWidget(newLabel);
        pwLayout->addWidget(newEdit);
        pwLayout->addWidget(confirmLabel);
        pwLayout->addWidget(confirmEdit);
        pwLayout->addWidget(errorLabel);
        
        QDialogButtonBox* pwButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, passwordDialog);
        pwLayout->addWidget(pwButtonBox);
        
        // Validation function
        auto validate = [oldEdit, newEdit, confirmEdit, errorLabel]() -> bool {
            QString oldPw = oldEdit->text();
            QString newPw = newEdit->text();
            QString confirmPw = confirmEdit->text();
            
            if (oldPw.isEmpty()) {
                errorLabel->setText("请输入原密码");
                return false;
            }
            if (newPw.isEmpty()) {
                errorLabel->setText("请输入新密码");
                return false;
            }
            if (newPw.length() < 6) {
                errorLabel->setText("新密码至少6位");
                return false;
            }
            if (confirmPw.isEmpty()) {
                errorLabel->setText("请确认新密码");
                return false;
            }
            if (newPw != confirmPw) {
                errorLabel->setText("两次密码不一致");
                return false;
            }
            if (newPw == oldPw) {
                errorLabel->setText("新密码不能与原密码相同");
                return false;
            }
            
            errorLabel->setText("");
            return true;
        };
        
        // Connect validation
        connect(newEdit, &QLineEdit::textChanged, [validate, oldEdit, confirmEdit, errorLabel]() {
            if (oldEdit->text().isEmpty() || confirmEdit->text().isEmpty()) return;
            validate();
        });
        connect(confirmEdit, &QLineEdit::textChanged, [validate, oldEdit, newEdit, errorLabel]() {
            if (oldEdit->text().isEmpty() || newEdit->text().isEmpty()) return;
            validate();
        });
        
        connect(pwButtonBox, &QDialogButtonBox::accepted, [this, oldEdit, newEdit, confirmEdit, pwLayout, passwordDialog, validate]() {
            if (!validate()) return;
            
            DatabaseManager* db = DatabaseManager::getInstance();
            QString oldPw = oldEdit->text();
            QString newPw = newEdit->text();
            
            if (db->authenticateUser(m_currentUser, oldPw)) {
                if (db->updateUserPassword(m_currentUser, newPw)) {
                    QMessageBox::information(passwordDialog, "成功", "密码修改成功");
                    // Clear fields
                    oldEdit->clear();
                    newEdit->clear();
                    confirmEdit->clear();
                } else {
                    QMessageBox::critical(passwordDialog, "错误", "密码更新失败");
                }
            } else {
                QMessageBox::critical(passwordDialog, "错误", "原密码错误");
            }
            passwordDialog->accept();
        });
        
        connect(pwButtonBox, &QDialogButtonBox::rejected, passwordDialog, &QDialog::reject);
        
        passwordDialog->exec();
        passwordDialog->deleteLater();
    });
    
    connect(buttonBox, &QDialogButtonBox::rejected, accountDialog, &QDialog::reject);
    
    // Show dialog
    accountDialog->exec();
    accountDialog->deleteLater();
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    
    if (m_isLoggedIn && !m_initialized) {
        loadSubWindowsToLayout();
        m_initialized = true;
    } else if (!m_isLoggedIn) {
    } else {
    }
}

void MainWindow::onNewSubWindowRefresh(int subId)
{
    if (!m_windowManager) {
        qWarning() << "MainWindow::onNewSubWindowRefresh: m_windowManager is null";
        return;
    }
    
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    if (!dbManager) {
        qWarning() << "MainWindow::onNewSubWindowRefresh: DatabaseManager is null";
        return;
    }
    
    QList<QJsonObject> allSubWindows = dbManager->getAllSubWindows();
    QJsonObject targetSubWindow;
    for (const QJsonObject& subWindow : allSubWindows) {
        if (subWindow["id"].toInt() == subId) {
            targetSubWindow = subWindow;
            break;
        }
    }
    
    if (targetSubWindow.isEmpty()) {
        qWarning() << "MainWindow::onNewSubWindowRefresh: No sub-window found for subId" << subId;
        return;
    }
    
    QString name = targetSubWindow["name"].toString();
    QString url = targetSubWindow["url"].toString();
    
    BrowserWidget* targetWidget = m_windowManager->findWidgetBySubId(subId);
    if (targetWidget) {
        targetWidget->setSubWindowId(subId);
        targetWidget->setSubWindowName(name);
        targetWidget->loadUrl(url);
    } else {
        qWarning() << "MainWindow::onNewSubWindowRefresh: No widget found for subId" << subId;
    }
}




