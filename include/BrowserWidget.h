#ifndef BROWSERWIDGET_H
#define BROWSERWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineHistory>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QJsonObject>
#include <QMouseEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

class BrowserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BrowserWidget(int windowId, QWidget *parent = nullptr);
    ~BrowserWidget();

    int getWindowId() const;
    QString getCurrentUrl() const;
    QString getCurrentTitle() const;
    void loadUrl(const QString& url);
    void setWindowId(int id);
    void setSubWindowName(const QString& name);
    QString getSubWindowName() const;
    void setSubWindowId(int subWindowId);
    int getSubWindowId() const;
    void saveState();
    void loadState();
    void saveCookies();
    void loadCookies();
    void clearLoginState();
    void setFullscreenMode(bool fullscreen);
    bool isFullscreenMode() const;
    void setShowBrowserUI(bool show);
    bool isShowBrowserUI() const;
    void setAllowResize(bool allow);
    bool isAllowResize() const;
    void updateWebViewResolution();
    double calculateOptimalZoomFactor() const;
    
    // Public interface methods
    void refresh();
    void stop();
    void goHome();

signals:
    void urlChanged(const QString& url);
    void titleChanged(const QString& title);
    void loadProgress(int progress);
    void loadFinished(bool success);
    void fullscreenRequested();
    void closeRequested();

private slots:
    void onUrlChanged(const QUrl& url);
    void onTitleChanged(const QString& title);
    void onLoadProgress(int progress);
    void onLoadFinished(bool success);
    void onBackClicked();
    void onForwardClicked();
    void onRefreshClicked();
    void onStopClicked();
    void onFullscreenClicked();
    void onCloseClicked();
    void onContextMenuRequested(const QPoint& pos);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupUI();
    void setupToolbar();
    void setupWebView();
    void setupContextMenu();
    void updateToolbarState();
    void saveWindowState();
    void loadWindowState();
    void addToHistory(const QString& url, const QString& title);
    bool isValidUrl(const QString& url);
    QString formatUrl(const QString& url);

    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_toolbarLayout;
    QLabel* m_subWindowNameLabel;
    QWebEngineProfile* m_profile;
    QWebEngineView* m_webView;
    QPushButton* m_fullscreenButton;
    QPushButton* m_refreshButton;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;

    // Context menu
    QMenu* m_contextMenu;
    QAction* m_backAction;
    QAction* m_forwardAction;
    QAction* m_refreshAction;
    QAction* m_stopAction;
    QAction* m_copyUrlAction;
    QAction* m_copyTitleAction;
    QAction* m_fullscreenAction;
    QAction* m_closeAction;

    // State
    int m_windowId;
    int m_subWindowId;  // ID from sub_windows table
    QString m_currentUrl;
    QString m_currentTitle;
    QString m_subWindowName;
    bool m_isFullscreen;
    bool m_showBrowserUI;
    bool m_allowResize;
    QJsonObject m_windowState;
    QTimer* m_saveTimer;
    
    // Resolution management
    double m_currentZoomFactor;
    QSize m_referenceSize;
    bool m_autoResolutionEnabled;
    
    // Hover button management
    QTimer* m_hoverTimer;
    QTimer* m_autoHideTimer;
    bool m_buttonsVisible;
    
    // Cookie file management
    bool saveCookiesToFile(const QString& cookieData);
    QString loadCookiesFromFile();
    QString getCookieFilePath();
    void executeCookieScript();
    void deleteCookieFile();
    void clearStorage();
    void showButtons();
    void hideButtons();
    
    // Pending cookie script to execute when page is ready
    QString m_pendingCookieScript;
    bool m_isLoaded = false;  // New: Track if URL is loaded
};

#endif // BROWSERWIDGET_H

