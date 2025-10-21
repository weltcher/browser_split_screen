#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QTranslator>
#include <QLibraryInfo>
#include <QWebEngineProfile>
#include "MainWindow.h"
#include "DatabaseManager.h"
#include <QGuiApplication>  // For setAttribute, if not already included
#include <QProcessEnvironment>  // Optional for env, but qputenv is in QtGlobal
#include <QCoreApplication> // Required for QCoreApplication::setAttribute

int main(int argc, char *argv[])
{
    // Qt WebEngine Configuration Adjustments - Disable GPU and network issues
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu --disable-software-rasterizer --no-sandbox --disable-gpu-sandbox --disable-web-security --ignore-certificate-errors --disable-features=VizDisplayCompositor --disable-background-timer-throttling --disable-history-quick-provider");
    qputenv("QT_LOGGING_RULES", "qt.webengine.*.debug=false;qt.webenginecontext.debug=false");  // Suppress warnings, but keep fatal
    qputenv("QTWEBENGINE_LOG_ENABLED", "true");  // FIXED: Enable stderr logging for Backend errors
    
    // Enable high DPI scaling for better Win10 compatibility
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Browser Split Screen");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("QunKong");
    app.setOrganizationDomain("qunkong.com");
    
    // Set application style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Initialize WebEngine
    QWebEngineProfile::defaultProfile()->setHttpUserAgent(
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 BrowserSplitScreen/1.0.0"
    );
    
    // Set dark theme palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    
    // Initialize database
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    if (!dbManager->initialize()) {
        QMessageBox::critical(nullptr, "Database Error", 
                            "Failed to initialize database. Please check file permissions.");
        return -1;
    }
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}

