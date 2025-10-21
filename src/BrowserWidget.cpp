#include "BrowserWidget.h"
#include "DatabaseManager.h"
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QShortcut>
#include <QNetworkCookie>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMouseEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

BrowserWidget::BrowserWidget(int windowId, QWidget *parent)
    : QWidget(parent)
    , m_windowId(windowId)
    , m_subWindowId(-1)  // Initialize to invalid ID
    , m_isFullscreen(false)
    , m_showBrowserUI(false)  // FIXED: Initialize to false to avoid immediate UI show crash
    , m_allowResize(false)
    , m_saveTimer(new QTimer(this))
    , m_currentZoomFactor(1.0)
    , m_referenceSize(1920, 1080)  // Default reference resolution
    , m_autoResolutionEnabled(true)
    , m_hoverTimer(new QTimer(this))
    , m_autoHideTimer(new QTimer(this))
    , m_buttonsVisible(false)
    , m_toolbarLayout(nullptr)  // FIXED: Explicitly initialize to null if not already
    , m_isLoaded(false)  // New: Initialize to false
{
    setupUI();

    setupContextMenu();
    
    setupToolbar();
    
    // Load saved state (but not cookies yet, as subWindowId is not set)
    loadWindowState();

    // Setup auto-save timer (save every 30 seconds)
    m_saveTimer->setInterval(30000);
    m_saveTimer->setSingleShot(false);
    connect(m_saveTimer, &QTimer::timeout, this, &BrowserWidget::saveState);
    m_saveTimer->start();
    
    // Setup mouse tracking and hover timers
    // setMouseTracking(true);  // Moved outside block for individual comment
    
    m_hoverTimer->setSingleShot(true);
    m_hoverTimer->setInterval(100); // 100ms delay before showing buttons
    connect(m_hoverTimer, &QTimer::timeout, this, &BrowserWidget::showButtons);
    
    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(2000); // Hide buttons after 2 seconds of no mouse movement
    connect(m_autoHideTimer, &QTimer::timeout, this, &BrowserWidget::hideButtons);
    
    // Setup keyboard shortcuts
    QShortcut* fullscreenShortcut = new QShortcut(QKeySequence("F11"), this);
    connect(fullscreenShortcut, &QShortcut::activated, this, &BrowserWidget::onFullscreenClicked);
    
    QShortcut* closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), this);
    connect(closeShortcut, &QShortcut::activated, this, &BrowserWidget::onCloseClicked);

    setMouseTracking(true);
}

BrowserWidget::~BrowserWidget()
{
    saveState();  // Save before destroy

    // FIXED: Flush Profile destruction to ensure HistoryBackend closes properly (fixes HasPath conflict for next profile)
    if (m_profile) {
        // Use Qt's destroy mechanism to flush Backend
        m_profile->deleteLater();  // Async delete with event loop flush
        // Optional: Set a task to ensure Backend shutdown
        m_profile->setHttpUserAgent("");  // Trigger cleanup
    }

    if (m_webView) {
        m_webView->deleteLater();
    }
}

void BrowserWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(2, 2, 2, 2);
    m_mainLayout->setSpacing(2);
    
    // Sub window name label
    m_subWindowNameLabel = new QLabel("子窗口", this);
    m_subWindowNameLabel->setStyleSheet("QLabel { background-color: #f0f0f0; border: 1px solid #ccc; padding: 2px; font-weight: bold; color: black; }");
    m_subWindowNameLabel->setAlignment(Qt::AlignCenter);
    m_subWindowNameLabel->setMaximumHeight(25);
    m_mainLayout->addWidget(m_subWindowNameLabel);
    
    setupWebView();
    
    // FIXED: Always create progressBar and statusLabel, even if toolbar is commented
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumHeight(3);
    m_mainLayout->addWidget(m_progressBar);
    
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setMaximumHeight(20);
    m_statusLabel->setStyleSheet("color: gray; font-size: 10px;");
    m_mainLayout->addWidget(m_statusLabel);
    
    // Set minimum size
    setMinimumSize(300, 200);
    
    // FIXED: Call with false initially, or comment if still issues
    // setShowBrowserUI(true);  // Temporarily commented; use false in constructor
}

void BrowserWidget::setupToolbar()
{
    // Create fullscreen button as floating button in top-right corner
    m_fullscreenButton = new QPushButton("⛶", this);
    m_fullscreenButton->setToolTip("全屏");
    m_fullscreenButton->setFixedSize(30, 30);
    m_fullscreenButton->setStyleSheet(
        "QPushButton { "
        "background-color: rgba(0, 0, 0, 0.3); "
        "border: 1px solid rgba(255, 255, 255, 0.5); "
        "border-radius: 15px; "
        "font-size: 14px; "
        "font-weight: bold; "
        "color: white; "
        "}"
        "QPushButton:hover { "
        "background-color: rgba(0, 0, 0, 0.5); "
        "border: 1px solid rgba(255, 255, 255, 0.8); "
        "}"
    );
    
    // Create refresh button as floating button in top-right corner
    m_refreshButton = new QPushButton("🔄", this);
    m_refreshButton->setToolTip("刷新");
    m_refreshButton->setFixedSize(30, 30);
    m_refreshButton->setStyleSheet(
        "QPushButton { "
        "background-color: rgba(0, 0, 0, 0.3); "
        "border: 1px solid rgba(255, 255, 255, 0.5); "
        "border-radius: 15px; "
        "font-size: 14px; "
        "font-weight: bold; "
        "color: white; "
        "}"
        "QPushButton:hover { "
        "background-color: rgba(0, 0, 0, 0.5); "
        "border: 1px solid rgba(255, 255, 255, 0.8); "
        "}"
    );
    
    // Position the buttons in top-right corner
    m_fullscreenButton->move(width() - 35, 5);
    m_refreshButton->move(width() - 70, 5);
    m_fullscreenButton->raise(); // Bring to front
    m_refreshButton->raise(); // Bring to front
    
    // Progress bar
    // m_progressBar = new QProgressBar(this); // Moved to setupUI
    // m_progressBar->setVisible(false);
    // m_progressBar->setMaximumHeight(3);
    // m_mainLayout->addWidget(m_progressBar);
    
    // Status label
    // m_statusLabel = new QLabel("就绪", this); // Moved to setupUI
    // m_statusLabel->setMaximumHeight(20);
    // m_statusLabel->setStyleSheet("color: gray; font-size: 10px;");
    // m_mainLayout->addWidget(m_statusLabel);
    
    // Connect button signals
    connect(m_fullscreenButton, &QPushButton::clicked, this, &BrowserWidget::onFullscreenClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &BrowserWidget::onRefreshClicked);
}

void BrowserWidget::setupWebView()
{
    
    // FIXED: Use unique in-memory Profile to avoid History DB lock/conflict (no persistent storage)
    static int s_profileCounter = 0;
    QString profileName = QString("BrowserWidget_%1_%2").arg(m_windowId).arg(++s_profileCounter);
    m_profile = new QWebEngineProfile(profileName, this);
    qDebug() << "BrowserWidget::setupWebView: Profile created:" << (m_profile ? "SUCCESS (unique in-memory)" : "FAILED");
    // FIXED: Temporarily disable persistent storage to prevent DB lock; restore later if needed
    // QString storagePath = QDir::homePath() + QString("/.browser_split_screen/profiles/%1").arg(m_windowId);
    // m_profile->setPersistentStoragePath(storagePath);
    
    m_webView = new QWebEngineView(m_profile, this);
    qDebug() << "BrowserWidget::setupWebView: WebView created:" << (m_webView ? "SUCCESS" : "FAILED");
    if (m_webView) {
        m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
    }
    
    if (m_webView) {
        QWebEngineSettings* settings = m_webView->settings();
        qDebug() << "BrowserWidget::setupWebView: Settings obtained:" << (settings ? "SUCCESS" : "FAILED");
        
        if (settings) {
            settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
            settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
            settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
            settings->setAttribute(QWebEngineSettings::AutoLoadImages, true);
            settings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
            settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
            settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
            settings->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, true);
            settings->setAttribute(QWebEngineSettings::TouchIconsEnabled, true);
            settings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
            settings->setAttribute(QWebEngineSettings::PrintElementBackgrounds, true);
            settings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
            settings->setAttribute(QWebEngineSettings::AllowGeolocationOnInsecureOrigins, true);
            settings->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, true);
            settings->setAttribute(QWebEngineSettings::ShowScrollBars, true);
            settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
            settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
            settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
            settings->setAttribute(QWebEngineSettings::LinksIncludedInFocusChain, true);
            settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
            qDebug() << "BrowserWidget::setupWebView: All web settings applied";
        }
    }
    
    m_mainLayout->addWidget(m_webView);
    
    // Connect web view signals
    connect(m_webView, &QWebEngineView::urlChanged, this, &BrowserWidget::onUrlChanged);
    connect(m_webView, &QWebEngineView::titleChanged, this, &BrowserWidget::onTitleChanged);
    connect(m_webView, &QWebEngineView::loadProgress, this, &BrowserWidget::onLoadProgress);
    connect(m_webView, &QWebEngineView::loadFinished, this, &BrowserWidget::onLoadFinished);
    connect(m_webView, &QWebEngineView::customContextMenuRequested, 
            this, &BrowserWidget::onContextMenuRequested);
    
}

void BrowserWidget::setupContextMenu()
{
    m_contextMenu = new QMenu(this);
    
    m_backAction = m_contextMenu->addAction("后退", this, &BrowserWidget::onBackClicked);
    m_forwardAction = m_contextMenu->addAction("前进", this, &BrowserWidget::onForwardClicked);
    m_contextMenu->addSeparator();
    m_refreshAction = m_contextMenu->addAction("刷新", this, &BrowserWidget::onRefreshClicked);
    m_stopAction = m_contextMenu->addAction("停止", this, &BrowserWidget::onStopClicked);
    m_contextMenu->addSeparator();
    m_copyUrlAction = m_contextMenu->addAction("复制网址", [this]() {
        QApplication::clipboard()->setText(m_currentUrl);
    });
    m_copyTitleAction = m_contextMenu->addAction("复制标题", [this]() {
        QApplication::clipboard()->setText(m_currentTitle);
    });
    m_contextMenu->addSeparator();
    m_fullscreenAction = m_contextMenu->addAction("全屏显示", this, &BrowserWidget::onFullscreenClicked);
    m_contextMenu->addSeparator();
    m_closeAction = m_contextMenu->addAction("关闭窗口", this, &BrowserWidget::onCloseClicked);
}

void BrowserWidget::onUrlChanged(const QUrl& url)
{
    m_currentUrl = url.toString();
    emit urlChanged(m_currentUrl);
    
    updateToolbarState();
}

void BrowserWidget::onTitleChanged(const QString& title)
{
    m_currentTitle = title;
    emit titleChanged(title);
    
    // Update window title if not empty
    if (!title.isEmpty() && title != "about:blank") {
        setWindowTitle(QString("窗口 %1 - %2").arg(m_windowId).arg(title));
    }
}

// FIXED: Enhance onLoadProgress with checks and logs
void BrowserWidget::onLoadProgress(int progress)
{
    if (m_progressBar) {
        m_progressBar->setValue(progress);
        m_progressBar->setVisible(progress < 100);
    }
    if (m_statusLabel) {
        m_statusLabel->setText(QString("加载中... %1%").arg(progress));
    }
    emit loadProgress(progress);
}

// FIXED: Enhance onLoadFinished with checks and logs
void BrowserWidget::onLoadFinished(bool success)
{
    if (m_progressBar) {
        m_progressBar->setVisible(false);
    }
    if (m_statusLabel) {
        m_statusLabel->setText(success ? "加载完成" : "加载失败");
    }
    
    if (success && !m_currentUrl.isEmpty()) {
        addToHistory(m_currentUrl, m_currentTitle);
        
        // Execute pending cookie script if any (for loading cookies)
        executeCookieScript();
        
        // Save cookies after successful page load (only if subWindowId is set)
        if (m_subWindowId > 0) {
            QTimer::singleShot(1000, this, &BrowserWidget::saveCookies);
        }
        m_isLoaded = true;  // Mark as loaded after success
    }
    
    emit loadFinished(success);
    updateToolbarState();
    
    // Update web view resolution after page loads
    if (success && m_autoResolutionEnabled && m_webView) {
        QTimer::singleShot(500, this, &BrowserWidget::updateWebViewResolution);
    }
}

void BrowserWidget::onFullscreenClicked()
{
    emit fullscreenRequested();
}

void BrowserWidget::onBackClicked()
{
    m_webView->back();
}

void BrowserWidget::onForwardClicked()
{
    m_webView->forward();
}

void BrowserWidget::onRefreshClicked()
{
    m_webView->reload();
}

void BrowserWidget::onStopClicked()
{
    m_webView->stop();
}

void BrowserWidget::onCloseClicked()
{
    emit closeRequested();
}


void BrowserWidget::onContextMenuRequested(const QPoint& pos)
{
    // Update context menu state
    m_backAction->setEnabled(m_webView->page()->history()->canGoBack());
    m_forwardAction->setEnabled(m_webView->page()->history()->canGoForward());
    m_stopAction->setEnabled(m_webView->page()->isLoading());
    
    m_contextMenu->exec(mapToGlobal(pos));
}

void BrowserWidget::updateToolbarState()
{
    // FIXED: Null check - if no toolbar, skip
    if (!m_toolbarLayout) {
        qDebug() << "updateToolbarState: m_toolbarLayout is null, nothing to update (setupToolbar commented)";
        return;
    }
    
    // No other buttons to update since we only have the fullscreen button
}

void BrowserWidget::saveWindowState()
{
    QJsonObject geometry;
    geometry["x"] = x();
    geometry["y"] = y();
    geometry["width"] = width();
    geometry["height"] = height();
    
    m_windowState = geometry;
}

// FIXED: Restore loadUrl in loadWindowState but with the delayed version from loadUrl
void BrowserWidget::loadWindowState()
{
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    qDebug() << "BrowserWidget::loadWindowState: DatabaseManager obtained:" << (dbManager ? "SUCCESS" : "FAILED");
    
    if (!dbManager) {
        qDebug() << "BrowserWidget::loadWindowState: ERROR - DatabaseManager is null, skipping";
        return;
    }
    
    QJsonObject config = dbManager->loadWindowConfig(m_windowId);
    
    if (config.contains("geometry")) {
        QJsonObject geometry = config["geometry"].toObject();
        if (geometry.contains("x") && geometry.contains("y") &&
            geometry.contains("width") && geometry.contains("height")) {
        }
    }
    
    // 从sub_window表获取URL进行加载，不再从window_configs获取
    // 需要有subWindowId，才能去数据库查sub_window表
    if (m_subWindowId > 0) {
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        if (dbManager) {
            QJsonObject subWindow = dbManager->getSubWindow(m_subWindowId);
            QString url = subWindow.value("url").toString();
            if (!url.isEmpty()) {
                loadUrl(url);
            } else {
            }
        } else {
            qDebug() << "BrowserWidget::loadWindowState: DatabaseManager unavailable, cannot fetch sub_window";
        }
    } else {
    }
    
}

void BrowserWidget::addToHistory(const QString& url, const QString& title)
{
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    dbManager->addHistoryRecord(url, title, m_windowId);
}

bool BrowserWidget::isValidUrl(const QString& url)
{
    QRegularExpression urlRegex(R"(^https?://.*)");
    return urlRegex.match(url).hasMatch();
}

// In formatUrl or default load (e.g., setSubWindowName if empty URL), temporarily use 'about:blank' for testing
QString BrowserWidget::formatUrl(const QString& url)
{
    if (url.isEmpty()) {
        return "about:blank";  // Keep for empty, but not force for baidu
    }
    
    if (isValidUrl(url)) {
        return url;
    }
    
    // If it looks like a domain, add https://
    QRegularExpression domainRegex(R"(^[a-zA-Z0-9][a-zA-Z0-9-]{1,61}[a-zA-Z0-9]\.[a-zA-Z]{2,}$)");
    if (domainRegex.match(url).hasMatch()) {
        return "https://" + url;
    }
    
    // Otherwise, treat as search query
    return "https://www.google.com/search?q=" + QUrl::toPercentEncoding(url);
}

void BrowserWidget::loadUrl(const QString& url)
{

    // Early return if already loaded with same URL
    QString formattedUrl = formatUrl(url);
    if (m_isLoaded && m_currentUrl == formattedUrl) {
        return;
    }

    // FIXED: Sanity check - ensure webview and page are valid before load
    if (!m_webView) {
        qDebug() << "loadUrl: CRITICAL - m_webView is null, cannot load URL";
        return;
    }
    if (!m_webView->page()) {
        qDebug() << "loadUrl: WARNING - m_webView->page() is null, delaying load";
        // Delay to allow page init
        QTimer::singleShot(100, [this, url]() { loadUrl(url); });
        return;
    }

    // Store URL for later loading if widget not visible yet
    m_currentUrl = formattedUrl;

    // PERFORMANCE: Only load if widget is visible (lazy loading strategy)
    if (!isVisible()) {
        qDebug() << "loadUrl: Widget not visible, deferring load until shown (lazy load)";
        m_isLoaded = false;  // Mark as not loaded so showEvent will trigger load
        return;
    }

    // FIXED: Delay load by 500ms to allow full WebEngine initialization (fixes connection_state crash)
    QTimer::singleShot(500, [this, formattedUrl]() {
        qDebug() << "loadUrl: Delayed load timer fired, calling m_webView->load";
        if (m_webView && m_webView->page()) {
            qDebug() << "loadUrl: Loading" << formattedUrl << "for widget" << m_windowId;
            m_webView->load(QUrl(formattedUrl));
        } else {
            qDebug() << "loadUrl: Delayed load failed - webview/page null";
        }
    });
    
}

void BrowserWidget::setWindowId(int id)
{
    m_windowId = id;
}

void BrowserWidget::saveState()
{
    saveWindowState();

    // Only save window_config if this widget has been assigned a subwindow
    // Use subWindowId as window_id for 1:1 mapping with sub_windows table
    if (m_subWindowId > 0) {
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        if (dbManager) {
            dbManager->saveWindowConfig(m_subWindowId, m_subWindowId, m_currentUrl, m_currentTitle, m_windowState);
        }
    } else {
    }

    // Also save cookies
    saveCookies();
}

void BrowserWidget::loadState()
{
    loadWindowState();
    
    // Also load cookies
    loadCookies();
}

void BrowserWidget::setFullscreenMode(bool fullscreen)
{
    m_isFullscreen = fullscreen;
    m_allowResize = fullscreen;
    
    // FIXED: Null checks for buttons
    if (m_fullscreenButton) {
        m_fullscreenButton->setText(fullscreen ? "取消全屏" : "⛶");
        m_fullscreenButton->setToolTip(fullscreen ? "退出全屏 (ESC)" : "全屏");
    } else {
        qDebug() << "setFullscreenMode: WARNING - m_fullscreenButton is null (setupToolbar commented)";
    }
    
    if (m_refreshButton) {
        // Update refresh button if needed
    } else {
        qDebug() << "setFullscreenMode: WARNING - m_refreshButton is null";
    }
    
    // In fullscreen mode, remove size constraints to allow full screen usage
    if (fullscreen) {
        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        
        // Hide buttons by default in fullscreen mode
        hideButtons();
    } else {
        // In normal mode, restore size constraints
        setMinimumSize(300, 200);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        // Show buttons in normal mode (directly set visible, don't use showButtons() which only works in fullscreen)
        if (m_fullscreenButton) {
            m_fullscreenButton->setVisible(true);
            m_fullscreenButton->setGraphicsEffect(nullptr);  // Remove any opacity effects
        }
        if (m_refreshButton) {
            m_refreshButton->setVisible(true);
            m_refreshButton->setGraphicsEffect(nullptr);  // Remove any opacity effects
        }
        m_buttonsVisible = true;
    }
    
    // Style the buttons differently in fullscreen mode
    if (fullscreen) {
        QString fullscreenButtonStyle = 
            "QPushButton { "
            "background-color: rgba(0, 0, 0, 0.8); "
            "color: white; "
            "border: 2px solid white; "
            "border-radius: 12px; "
            "font-size: 12px; "
            "font-weight: bold; "
            "width: 25px; "
            "height: 25px; "
            "padding: 0px; "
            "}"
            "QPushButton:hover { "
            "background-color: rgba(0, 0, 0, 1.0); "
            "}";
        
        if (m_fullscreenButton) m_fullscreenButton->setStyleSheet(fullscreenButtonStyle);
        if (m_refreshButton) m_refreshButton->setStyleSheet(fullscreenButtonStyle);
        
        // Change fullscreen button text to exit fullscreen icon
        if (m_fullscreenButton) m_fullscreenButton->setText("✕");
        if (m_fullscreenButton) m_fullscreenButton->setToolTip("退出全屏");
        
        // In fullscreen mode, position buttons in top-right corner of the screen with more margin
        // Keep proper distance between refresh and fullscreen buttons (25px width + 5px gap)
        if (m_fullscreenButton) m_fullscreenButton->move(width() - 35, 15);
        if (m_refreshButton) m_refreshButton->move(width() - 65, 15); // 35 + 25 + 5 = 65
    } else {
        QString normalButtonStyle = 
            "QPushButton { "
            "background-color: rgba(0, 0, 0, 0.3); "
            "border: 1px solid rgba(255, 255, 255, 0.5); "
            "border-radius: 15px; "
            "font-size: 14px; "
            "font-weight: bold; "
            "color: white; "
            "}"
            "QPushButton:hover { "
            "background-color: rgba(0, 0, 0, 0.5); "
            "border: 1px solid rgba(255, 255, 255, 0.8); "
            "}";
        
        if (m_fullscreenButton) m_fullscreenButton->setStyleSheet(normalButtonStyle);
        if (m_refreshButton) m_refreshButton->setStyleSheet(normalButtonStyle);
        
        // Restore fullscreen button text to enter fullscreen icon
        if (m_fullscreenButton) m_fullscreenButton->setText("⛶");
        if (m_fullscreenButton) m_fullscreenButton->setToolTip("全屏");
        
        // In normal mode, position buttons in top-right corner of the widget
        if (m_fullscreenButton) m_fullscreenButton->move(width() - 35, 5);
        if (m_refreshButton) m_refreshButton->move(width() - 70, 5);
    }
    
    // Always bring buttons to front
    if (m_fullscreenButton) m_fullscreenButton->raise();
    if (m_refreshButton) m_refreshButton->raise();
    
    // Show/hide sub window name label based on fullscreen mode
    if (m_subWindowNameLabel) {
        m_subWindowNameLabel->setVisible(!fullscreen);
    }
    
    // Show/hide status label based on fullscreen mode
    if (m_statusLabel) {
        m_statusLabel->setVisible(!fullscreen);
    }
    
    // Show/hide progress bar based on fullscreen mode
    if (m_progressBar) {
        m_progressBar->setVisible(!fullscreen);
    }
    
    // Update web view resolution when switching fullscreen mode
    if (m_autoResolutionEnabled) {
        QTimer::singleShot(200, this, &BrowserWidget::updateWebViewResolution);
    }
}

bool BrowserWidget::isFullscreenMode() const
{
    return m_isFullscreen;
}

int BrowserWidget::getWindowId() const
{
    return m_windowId;
}

QString BrowserWidget::getCurrentUrl() const
{
    return m_currentUrl;
}

QString BrowserWidget::getCurrentTitle() const
{
    return m_currentTitle;
}

void BrowserWidget::refresh()
{
    m_webView->reload();
}

void BrowserWidget::stop()
{
    m_webView->stop();
}

void BrowserWidget::goHome()
{
    m_webView->load(QUrl("https://www.baidu.com"));
}

void BrowserWidget::setSubWindowName(const QString& name)
{
    m_subWindowName = name;
    if (m_subWindowNameLabel) {
        m_subWindowNameLabel->setText(name.isEmpty() ? "子窗口" : name);
    }
    
    // Lazy load if name set and not loaded
    if (!m_isLoaded && !name.isEmpty() && !m_currentUrl.isEmpty()) {
        loadUrl(m_currentUrl);
    }
}

QString BrowserWidget::getSubWindowName() const
{
    return m_subWindowName;
}

void BrowserWidget::setShowBrowserUI(bool show)
{
    m_showBrowserUI = show;
    
    // FIXED: Null check for m_toolbarLayout before loop
    if (m_toolbarLayout) {
        for (int i = 0; i < m_toolbarLayout->count(); ++i) {
            QWidget* widget = m_toolbarLayout->itemAt(i)->widget();
            if (widget) {
                widget->setVisible(show);
            } else {
            }
        }
    } else {
        qDebug() << "setShowBrowserUI: m_toolbarLayout is null (setupToolbar commented), skipping toolbar loop";
    }
    
    // FIXED: Always handle progressBar and statusLabel with null checks
    if (m_progressBar) {
        m_progressBar->setVisible(show);
    } else {
        qDebug() << "setShowBrowserUI: WARNING - m_progressBar is null";
    }
    
    if (m_statusLabel) {
        m_statusLabel->setVisible(show);
    } else {
        qDebug() << "setShowBrowserUI: WARNING - m_statusLabel is null";
    }
    
    // Always show sub window name label (as before)
    if (m_subWindowNameLabel) {
        m_subWindowNameLabel->setVisible(true);
    }
    
}

bool BrowserWidget::isShowBrowserUI() const
{
    return m_showBrowserUI;
}

void BrowserWidget::setAllowResize(bool allow)
{
    m_allowResize = allow;
}

bool BrowserWidget::isAllowResize() const
{
    return m_allowResize;
}

void BrowserWidget::updateWebViewResolution()
{
    if (!m_webView || !m_autoResolutionEnabled) {
        return;
    }
    
    double zoomFactor = calculateOptimalZoomFactor();
    if (qAbs(zoomFactor - m_currentZoomFactor) > 0.01) {  // Only update if significant change
        m_currentZoomFactor = zoomFactor;
        m_webView->setZoomFactor(zoomFactor);
    }
}

double BrowserWidget::calculateOptimalZoomFactor() const
{
    if (!m_webView) {
        return 1.0;
    }
    
    QSize currentSize = size();
    if (currentSize.width() <= 0 || currentSize.height() <= 0) {
        return 1.0;
    }
    
    // Calculate zoom factor based on current window size vs reference size
    double widthRatio = static_cast<double>(currentSize.width()) / m_referenceSize.width();
    double heightRatio = static_cast<double>(currentSize.height()) / m_referenceSize.height();
    
    // Use the smaller ratio to ensure content fits within the window
    double baseZoomFactor = qMin(widthRatio, heightRatio);
    
    // Apply additional scaling based on fullscreen mode
    if (m_isFullscreen) {
        // In fullscreen mode, we want to use more of the screen space
        // Scale up by a factor that makes better use of the screen
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QSize screenSize = screen->geometry().size();
            double screenWidthRatio = static_cast<double>(screenSize.width()) / m_referenceSize.width();
            double screenHeightRatio = static_cast<double>(screenSize.height()) / m_referenceSize.height();
            double screenZoomFactor = qMin(screenWidthRatio, screenHeightRatio);
            
            // Use a weighted average between base zoom and screen zoom
            baseZoomFactor = (baseZoomFactor * 0.3) + (screenZoomFactor * 0.7);
        }
    } else {
        // In normal mode, apply a slight scaling to make content more readable
        baseZoomFactor *= 0.8;  // Slightly smaller to fit more content
    }

    // Clamp zoom factor to reasonable bounds
    baseZoomFactor = qBound(0.25, baseZoomFactor, 3.0);
    
    return baseZoomFactor;
}

void BrowserWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    // FIXED: Null checks before positioning
    if (m_fullscreenButton) {
        if (m_isFullscreen) {
            m_fullscreenButton->move(width() - 35, 15);
        } else {
            m_fullscreenButton->move(width() - 35, 5);
        }
        m_fullscreenButton->raise();
    } else {
        qDebug() << "resizeEvent: m_fullscreenButton null, skipping";
    }
    
    if (m_refreshButton) {
        if (m_isFullscreen) {
            m_refreshButton->move(width() - 65, 15);
        } else {
            m_refreshButton->move(width() - 70, 5);
        }
        m_refreshButton->raise();
    } else {
        qDebug() << "resizeEvent: m_refreshButton null, skipping";
    }
    
    // FIXED: Update resolution only if webview exists
    if (m_webView && m_autoResolutionEnabled) {
        QTimer::singleShot(100, this, &BrowserWidget::updateWebViewResolution);
    } else {
    }
}

void BrowserWidget::setSubWindowId(int subWindowId)
{
    m_subWindowId = subWindowId;
    
    // Load cookies for this sub window when ID is set
    if (m_subWindowId > 0) {
        loadCookies();
    } else {
        qDebug() << "BrowserWidget::setSubWindowId: Invalid subWindowId" << subWindowId << ", skipping cookie load";
    }
}

int BrowserWidget::getSubWindowId() const
{
    return m_subWindowId;
}

void BrowserWidget::saveCookies()
{
    if (m_subWindowId <= 0 || !m_webView || !m_profile) {
        qDebug() << "BrowserWidget::saveCookies: Skipping cookie save - subWindowId:" << m_subWindowId << "webView:" << (m_webView ? "valid" : "null") << "profile:" << (m_profile ? "valid" : "null");
        return;
    }
    
    
    // Get all cookies from the profile's cookie store
    QWebEngineCookieStore* cookieStore = m_profile->cookieStore();
    if (!cookieStore) {
        qDebug() << "BrowserWidget::saveCookies: Cookie store is null";
        return;
    }
    
    // Use JavaScript to get cookies from the current page (more reliable than cookie store API)
    QString script = R"(
        (function() {
            var cookies = document.cookie.split(';');
            var cookieArray = [];
            for (var i = 0; i < cookies.length; i++) {
                var cookie = cookies[i].trim();
                if (cookie) {
                    var parts = cookie.split('=');
                    if (parts.length >= 2) {
                        var name = parts[0];
                        var value = parts.slice(1).join('=');
                        cookieArray.push({
                            name: name,
                            value: value,
                            domain: window.location.hostname,
                            path: window.location.pathname,
                            url: window.location.href
                        });
                    }
                }
            }
            return JSON.stringify(cookieArray);
        })()
    )";
    
    m_webView->page()->runJavaScript(script, [this](const QVariant& result) {
        if (result.isValid()) {
            QString cookieData = result.toString();
            
            // Save to file
            if (saveCookiesToFile(cookieData)) {
            } else {
                qDebug() << "BrowserWidget::saveCookies: Failed to save cookies for sub window" << m_subWindowId;
            }
        } else {
            qDebug() << "BrowserWidget::saveCookies: Failed to get cookie data for sub window" << m_subWindowId;
        }
    });
}

void BrowserWidget::loadCookies()
{
    if (m_subWindowId <= 0 || !m_webView || !m_profile) {
        return;
    }

    // Load cookies from file
    QString cookieData = loadCookiesFromFile();
    
    if (cookieData.isEmpty()) {
        return;
    }
    
    
    // Parse cookies from JSON
    QJsonDocument doc = QJsonDocument::fromJson(cookieData.toUtf8());
    if (!doc.isArray()) {
        qDebug() << "BrowserWidget::loadCookies: Invalid cookie data format for sub window" << m_subWindowId;
        qDebug() << "=== BrowserWidget::loadCookies END (invalid format) ===";
        return;
    }
    
    QJsonArray cookieArray = doc.array();
    
    // Build JavaScript to set cookies
    QString script = "document.cookie = ''; "; // Clear existing cookies first
    
    for (const QJsonValue& value : cookieArray) {
        if (!value.isObject()) continue;
        
        QJsonObject cookieObj = value.toObject();
        QString name = cookieObj["name"].toString();
        QString valueStr = cookieObj["value"].toString();
        QString domain = cookieObj["domain"].toString();
        QString path = cookieObj["path"].toString();
        
        
        // Build cookie string
        QString cookieString = QString("%1=%2").arg(name, valueStr);
        if (!domain.isEmpty()) {
            cookieString += QString("; domain=%1").arg(domain);
        }
        if (!path.isEmpty()) {
            cookieString += QString("; path=%1").arg(path);
        }
        
        script += QString("document.cookie = '%1'; ").arg(cookieString);
    }
    
    
    // Store the script for later execution when page is ready
    m_pendingCookieScript = script;
    
    // If page is already loaded and not loading, execute immediately
    if (m_webView->page()->url().isValid() && !m_webView->page()->isLoading()) {
        executeCookieScript();
    } else {
    }
}

QString BrowserWidget::getCookieFilePath()
{
    // Create cookies directory in user's home directory
    QString homeDir = QDir::homePath();
    QString cookiesDir = QDir(homeDir).filePath(".browser_split_screen/cookies");
    
    // Ensure directory exists
    QDir().mkpath(cookiesDir);
    
    // Return file path for this sub window
    QString filePath = QDir(cookiesDir).filePath(QString("cookies_%1.json").arg(m_subWindowId));
    return filePath;
}

bool BrowserWidget::saveCookiesToFile(const QString& cookieData)
{
    if (m_subWindowId <= 0) {
        return false;
    }
    
    QString filePath = getCookieFilePath();
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "BrowserWidget::saveCookiesToFile: Failed to open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << cookieData;
    
    file.close();
    return true;
}

QString BrowserWidget::loadCookiesFromFile()
{
    if (m_subWindowId <= 0) {
        qDebug() << "BrowserWidget::loadCookiesFromFile: Invalid subWindowId, returning empty";
        qDebug() << "=== BrowserWidget::loadCookiesFromFile END (invalid ID) ===";
        return QString();
    }
    
    QString filePath = getCookieFilePath();
    
    QFile file(filePath);
    
    if (!file.exists()) {
        return QString();
    }
    
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "BrowserWidget::loadCookiesFromFile: Failed to open file for reading:" << filePath << "Error:" << file.errorString();
        qDebug() << "=== BrowserWidget::loadCookiesFromFile END (open failed) ===";
        return QString();
    }
    
    
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString cookieData = in.readAll();
    
    file.close();
    return cookieData;
}

void BrowserWidget::executeCookieScript()
{
    if (!m_pendingCookieScript.isEmpty()) {
        m_webView->page()->runJavaScript(m_pendingCookieScript, [this](const QVariant& result) {
        });
        m_pendingCookieScript.clear();
    }
}

void BrowserWidget::clearLoginState()
{
    
    // Clear all cookies from the current page using JavaScript
    QString clearCookiesScript = R"(
        (function() {
            // Get all cookies
            var cookies = document.cookie.split(';');
            for (var i = 0; i < cookies.length; i++) {
                var cookie = cookies[i].trim();
                if (cookie) {
                    var parts = cookie.split('=');
                    if (parts.length >= 2) {
                        var name = parts[0];
                        // Clear cookie by setting it to expire in the past
                        document.cookie = name + '=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;';
                        document.cookie = name + '=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/; domain=' + window.location.hostname;
                        document.cookie = name + '=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/; domain=.' + window.location.hostname;
                    }
                }
            }
            return 'Cookies cleared';
        })()
    )";
    
    m_webView->page()->runJavaScript(clearCookiesScript, [this](const QVariant& result) {
        
        // Clear cookies from profile's cookie store
        if (m_profile && m_profile->cookieStore()) {
            m_profile->cookieStore()->deleteAllCookies();
        }
        
        // Delete cookie file
        deleteCookieFile();
        
        // Clear local storage and session storage
        clearStorage();
        
        // Reload the page to reflect logout state
        m_webView->reload();
        
    });
}

void BrowserWidget::deleteCookieFile()
{
    if (m_subWindowId <= 0) {
        return;
    }
    
    QString filePath = getCookieFilePath();
    QFile file(filePath);
    
    if (file.exists()) {
        if (file.remove()) {
        } else {
            qDebug() << "BrowserWidget::deleteCookieFile: Failed to delete cookie file for sub window" << m_subWindowId << ":" << filePath;
        }
    } else {
    }
}

void BrowserWidget::clearStorage()
{
    // Clear local storage and session storage using JavaScript
    QString clearStorageScript = R"(
        (function() {
            try {
                // Clear local storage
                localStorage.clear();
                // Clear session storage
                sessionStorage.clear();
                return 'Storage cleared';
            } catch (e) {
                return 'Error clearing storage: ' + e.message;
            }
        })()
    )";
    
    m_webView->page()->runJavaScript(clearStorageScript, [this](const QVariant& result) {
    });
}

void BrowserWidget::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);
    
    // Only handle mouse tracking in fullscreen mode
    if (!m_isFullscreen) {
        return;
    }
    
    QPoint mousePos = event->pos();
    int topBoundary = 50; // Top 50 pixels of the screen
    
    // Check if mouse is in the top boundary area
    if (mousePos.y() <= topBoundary) {
        // Mouse is near the top, start hover timer to show buttons
        if (!m_buttonsVisible) {
            m_hoverTimer->start();
        }
        // Reset auto-hide timer
        m_autoHideTimer->start();
    } else {
        // Mouse is not in top area, hide buttons if they're visible
        if (m_buttonsVisible) {
            m_autoHideTimer->start();
        }
        // Stop hover timer
        m_hoverTimer->stop();
    }
}

void BrowserWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_isLoaded && !m_currentUrl.isEmpty()) {
        loadUrl(m_currentUrl);
    }
}

void BrowserWidget::showButtons()
{
    if (!m_isFullscreen || m_buttonsVisible) {
        return;
    }
    
    m_buttonsVisible = true;
    
    // Show buttons with fade-in animation
    if (m_fullscreenButton) m_fullscreenButton->setVisible(true);
    if (m_refreshButton) m_refreshButton->setVisible(true);
    
    // Apply fade-in effect using opacity
    QGraphicsOpacityEffect* fullscreenEffect = new QGraphicsOpacityEffect(this);
    QGraphicsOpacityEffect* refreshEffect = new QGraphicsOpacityEffect(this);
    
    if (m_fullscreenButton) m_fullscreenButton->setGraphicsEffect(fullscreenEffect);
    if (m_refreshButton) m_refreshButton->setGraphicsEffect(refreshEffect);
    
    // Animate opacity from 0 to 1
    QPropertyAnimation* fullscreenAnim = new QPropertyAnimation(fullscreenEffect, "opacity", this);
    QPropertyAnimation* refreshAnim = new QPropertyAnimation(refreshEffect, "opacity", this);
    
    fullscreenAnim->setDuration(200);
    fullscreenAnim->setStartValue(0.0);
    fullscreenAnim->setEndValue(1.0);
    
    refreshAnim->setDuration(200);
    refreshAnim->setStartValue(0.0);
    refreshAnim->setEndValue(1.0);
    
    fullscreenAnim->start();
    refreshAnim->start();
    
    // Start auto-hide timer
    m_autoHideTimer->start();
}

void BrowserWidget::hideButtons()
{
    if (!m_isFullscreen || !m_buttonsVisible) {
        return;
    }
    
    m_buttonsVisible = false;
    
    // Get current opacity effects
    QGraphicsOpacityEffect* fullscreenEffect = qobject_cast<QGraphicsOpacityEffect*>(m_fullscreenButton->graphicsEffect());
    QGraphicsOpacityEffect* refreshEffect = qobject_cast<QGraphicsOpacityEffect*>(m_refreshButton->graphicsEffect());
    
    if (fullscreenEffect && refreshEffect) {
        // Animate opacity from current to 0
        QPropertyAnimation* fullscreenAnim = new QPropertyAnimation(fullscreenEffect, "opacity", this);
        QPropertyAnimation* refreshAnim = new QPropertyAnimation(refreshEffect, "opacity", this);
        
        fullscreenAnim->setDuration(200);
        fullscreenAnim->setStartValue(fullscreenEffect->opacity());
        fullscreenAnim->setEndValue(0.0);
        
        refreshAnim->setDuration(200);
        refreshAnim->setStartValue(refreshEffect->opacity());
        refreshAnim->setEndValue(0.0);
        
        // Hide buttons after animation completes
        connect(fullscreenAnim, &QPropertyAnimation::finished, [this]() {
            if (m_fullscreenButton) m_fullscreenButton->setVisible(false);
        });
        connect(refreshAnim, &QPropertyAnimation::finished, [this]() {
            if (m_refreshButton) m_refreshButton->setVisible(false);
        });
        
        fullscreenAnim->start();
        refreshAnim->start();
    } else {
        // No animation, just hide
        if (m_fullscreenButton) m_fullscreenButton->setVisible(false);
        if (m_refreshButton) m_refreshButton->setVisible(false);
    }
}



