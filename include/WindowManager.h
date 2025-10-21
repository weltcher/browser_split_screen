#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QObject>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QWidget>
#include <QList>
#include <QJsonObject>
#include <QTimer>
#include "BrowserWidget.h"

class WindowManager : public QObject
{
    Q_OBJECT

public:
    explicit WindowManager(QWidget* parentWidget, QObject *parent = nullptr);
    ~WindowManager();

    void setLayout(int windowCount);
    void setLayout(int windowCount, bool forceUpdate);
    int getCurrentWindowCount() const;
    QList<BrowserWidget*> getBrowserWidgets() const;
    BrowserWidget* getBrowserWidget(int index) const;
    void addBrowserWidget();
    void removeBrowserWidget(int index);
    void clearAllWidgets();
    void saveAllStates();
    void loadAllStates();
    void setParentWidget(QWidget* parent);
    void detachWidgetFromLayout(BrowserWidget* widget);
    void attachWidgetToLayout(BrowserWidget* widget, int position);
    void forceLayoutUpdate();
    void synchronizeWidgetWidths();
    void ensureConsistentWidths();
    void setColumnCount(int columns);
    int getColumnCount() const;
    void updateWidgetContent(int index, int subId, const QString& name, const QString& url);  // New: Update pooled widget content
    BrowserWidget* findWidgetBySubId(int subId) const;

    // Layout configurations
    static const QList<int> SUPPORTED_WINDOW_COUNTS;
    static QPair<int, int> getGridDimensions(int windowCount);
    static bool isValidWindowCount(int windowCount);

signals:
    void allWidgetsCreated();  // FIXED: Signal emitted when all delayed widgets are created and ready
    void layoutChanged(int windowCount);
    void widgetAdded(BrowserWidget* widget);
    void widgetRemoved(int index);
    void fullscreenRequested(BrowserWidget* widget);

private slots:
    void onWidgetFullscreenRequested();
    void onWidgetCloseRequested();
    void onAutoSave();
    void onParentWidgetResized();

private:
    void setupLayout();
    void updateLayout();
    // void createBrowserWidgets();  // Commented out: No longer needed with pool
    void destroyBrowserWidgets();
    void connectWidgetSignals(BrowserWidget* widget);
    void disconnectWidgetSignals(BrowserWidget* widget);
    int calculateDynamicWidth() const;

    QWidget* m_parentWidget;
    QGridLayout* m_gridLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_scrollContent;
    QVBoxLayout* m_verticalLayout;
    QList<BrowserWidget*> m_browserWidgets;
    int m_currentWindowCount;
    int m_columnCount;
    QTimer* m_autoSaveTimer;
    QTimer* m_widthSyncTimer;
    
    // Fixed widget dimensions
    static const int FIXED_WIDGET_WIDTH;
    static const int FIXED_WIDGET_HEIGHT;
    
    // Calculate height based on 5:3 aspect ratio
    int calculateHeightFromWidth(int width) const;
};

#endif // WINDOWMANAGER_H

