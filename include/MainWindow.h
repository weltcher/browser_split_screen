#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QStackedWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QPushButton>
#include "LoginDialog.h"
#include "WindowManager.h"
#include "BrowserWidget.h"
#include "DatabaseManager.h"
#include "SubWindowManager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onAllWidgetsCreated();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    // Menu actions
    void onExit();
    void onRefreshAll();
    void onSubWindowManager();
    void onSettings();
    void onAbout();
    void onLogout();
    void onAccountManager();
    
    // 布局现在由子窗口数据自动决定
    
    // Window management
    void onFullscreenRequested(BrowserWidget* widget);
    void onWindowCloseRequested();
    
    // SubWindow management
    void onSubWindowAdded(const QJsonObject& subWindow);
    void onNewSubWindowRefresh(int subId);
    void onSubWindowUpdated(const QJsonObject& subWindow);
    void onSubWindowDeleted(int subWindowId);
    
    // Status updates
    void updateStatusBar();
    void onAutoSave();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void setupLogoutWidget();
    void setupConnections();
    void setupShortcuts();
    
    bool checkLogin();
    void showLoginDialog();
    void handleLogin();
    void handleLogout();
    void saveSettings();
    void loadSettings();
    void showFullscreenWindow(BrowserWidget* widget);
    void hideFullscreenWindow();
    void showLogoutPage();
    void showMainInterface();
    void loadSubWindowsToLayout();
    void applyWindowCount(int windowCount);
    void applyWindowColumns(int columns);
    int calculateRequiredWindowWidth(int columns);
    void centerWindowOnScreen();
    
    // UI Components
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;
    QStatusBar* m_statusBar;
    
    // Central widget
    QWidget* m_centralWidget;
    QVBoxLayout* m_centralLayout;
    QStackedWidget* m_stackedWidget;
    QWidget* m_mainWidget;
    QWidget* m_logoutWidget;
    QWidget* m_fullscreenWidget;
    QLabel* m_emptyStateLabel;
    QPushButton* m_logoutLoginButton;
    
    // Window management
    WindowManager* m_windowManager;
    BrowserWidget* m_fullscreenBrowser;
    int m_fullscreenWidgetPosition;
    
    // Menu actions
    QAction* m_logoutAction;
    QAction* m_exitAction;
    QAction* m_refreshAllAction;
    QAction* m_subWindowManagerAction;
    QAction* m_settingsAction;
    QAction* m_aboutAction;
    QAction* m_accountManagerAction;
    
    // 布局现在由子窗口数据自动决定
    QLabel* m_statusLabel;
    QProgressBar* m_globalProgressBar;
    
    // Login dialog
    LoginDialog* m_loginDialog;
    
    // SubWindow manager
    SubWindowManager* m_subWindowManager;
    
    // State
    bool m_isLoggedIn;
    QString m_currentUser;
    int m_currentLayout;
    QTimer* m_autoSaveTimer;
    bool m_initialized{false};
    bool m_loadingSubwindows = false;  // FIXED: Guard to prevent recursive/infinite loading calls
};

#endif // MAINWINDOW_H
