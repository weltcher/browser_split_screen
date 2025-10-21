#include "WindowManager.h"
#include <QApplication>
#include <QDebug>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include "BrowserWidget.h"  // Ensure included for BrowserWidget*

const QList<int> WindowManager::SUPPORTED_WINDOW_COUNTS = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
const int WindowManager::FIXED_WIDGET_WIDTH = 500;  // 固定宽度
const int WindowManager::FIXED_WIDGET_HEIGHT = 300; // 固定高度

// 根据5:3比例计算高度
int WindowManager::calculateHeightFromWidth(int width) const
{
    return static_cast<int>(width * 3.0 / 5.0);
}

WindowManager::WindowManager(QWidget* parentWidget, QObject *parent)
    : QObject(parent)
    , m_parentWidget(parentWidget)
    , m_gridLayout(nullptr)
    , m_scrollArea(nullptr)
    , m_scrollContent(nullptr)
    , m_verticalLayout(nullptr)
    , m_currentWindowCount(0)
    , m_columnCount(2)  // Default to 2 columns
    , m_autoSaveTimer(new QTimer(this))
    , m_widthSyncTimer(new QTimer(this))
{
    setupLayout();
    
    // Pre-create fixed pool of 16 BrowserWidgets for reuse (optimization)
    for(int i = 1; i <= 16; i++) {
        BrowserWidget* w = new BrowserWidget(i, m_parentWidget);
        m_browserWidgets.append(w);
        connectWidgetSignals(w);
        w->hide();  // Initially hidden
    }
    m_currentWindowCount = 0;  // Start with 0 visible
    updateLayout();
    
    // Setup auto-save timer (save every 60 seconds)
    m_autoSaveTimer->setInterval(60000);
    m_autoSaveTimer->setSingleShot(false);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &WindowManager::onAutoSave);
    m_autoSaveTimer->start();
    
    // Setup width synchronization timer (check every 2 seconds)
    m_widthSyncTimer->setInterval(2000);
    m_widthSyncTimer->setSingleShot(false);
    connect(m_widthSyncTimer, &QTimer::timeout, this, &WindowManager::onParentWidgetResized);
    m_widthSyncTimer->start();
}

WindowManager::~WindowManager()
{
    saveAllStates();
    destroyBrowserWidgets();
}

void WindowManager::setupLayout()
{
    if (!m_parentWidget) {
        return;
    }
    
    // Create scroll area if it doesn't exist
    if (!m_scrollArea) {
        m_scrollArea = new QScrollArea(m_parentWidget);
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_scrollArea->setFrameShape(QFrame::NoFrame);
        
        // Create content widget for scroll area
        m_scrollContent = new QWidget();
        m_scrollArea->setWidget(m_scrollContent);
        
        // Create vertical layout for content
        m_verticalLayout = new QVBoxLayout(m_scrollContent);
        m_verticalLayout->setContentsMargins(5, 5, 5, 5);
        m_verticalLayout->setSpacing(5);
        m_verticalLayout->setAlignment(Qt::AlignTop);
        
        // Create main layout for parent widget
        if (!m_gridLayout) {
            m_gridLayout = new QGridLayout(m_parentWidget);
            m_gridLayout->setContentsMargins(0, 0, 0, 0);
            m_gridLayout->setSpacing(0);
            m_gridLayout->addWidget(m_scrollArea, 0, 0);
        }
    }
    
    updateLayout();
}

void WindowManager::setLayout(int windowCount)
{
    setLayout(windowCount, false);
}

void WindowManager::setLayout(int windowCount, bool forceUpdate)
{
    if (!isValidWindowCount(windowCount)) {
        qWarning() << "Unsupported window count:" << windowCount;
        return;
    }
    
    if (m_currentWindowCount == windowCount && !forceUpdate) {
        return;
    }
    
    // No saveAllStates() or destroy - reuse pool
    m_currentWindowCount = windowCount;
    updateLayout();
    
    emit layoutChanged(windowCount);
    if (windowCount > 0 && m_browserWidgets.size() >= windowCount) {
        emit allWidgetsCreated();
    }
}

int WindowManager::getCurrentWindowCount() const
{
    return m_currentWindowCount;
}

QList<BrowserWidget*> WindowManager::getBrowserWidgets() const
{
    return m_browserWidgets;
}

BrowserWidget* WindowManager::getBrowserWidget(int index) const
{
    if (index >= 0 && index < m_browserWidgets.size()) {
        return m_browserWidgets[index];
    }
    return nullptr;
}

void WindowManager::addBrowserWidget()
{
    if (m_browserWidgets.size() >= 16) {
        qWarning() << "Maximum number of browser widgets reached (16)";
        return;
    }
    
    int newIndex = m_browserWidgets.size();
    BrowserWidget* newWidget = new BrowserWidget(newIndex + 1, m_parentWidget);
    
    m_browserWidgets.append(newWidget);
    connectWidgetSignals(newWidget);
    
    // Set widget size based on column count
    int widgetWidth = (m_columnCount == 1) ? 880 : 500;
    int widgetHeight = calculateHeightFromWidth(widgetWidth); // 根据5:3比例计算高度
    newWidget->setFixedSize(widgetWidth, widgetHeight);
    
    // Rebuild the entire layout to maintain proper row structure
    updateLayout();
    
    emit widgetAdded(newWidget);
}

void WindowManager::removeBrowserWidget(int index)
{
    if (index < 0 || index >= m_browserWidgets.size()) {
        return;
    }
    
    BrowserWidget* widget = m_browserWidgets[index];
    disconnectWidgetSignals(widget);
    
    // Remove from list and delete
    m_browserWidgets.removeAt(index);
    widget->deleteLater();
    
    // Update window IDs for remaining widgets
    for (int i = index; i < m_browserWidgets.size(); ++i) {
        m_browserWidgets[i]->setWindowId(i + 1);
    }
    
    // Rebuild the entire layout to maintain proper row structure
    updateLayout();
    
    emit widgetRemoved(index);
}

void WindowManager::clearAllWidgets()
{
    destroyBrowserWidgets();
    m_currentWindowCount = 0;
    updateLayout();
}

void WindowManager::saveAllStates()
{
    for (BrowserWidget* widget : m_browserWidgets) {
        widget->saveState();
    }
}

void WindowManager::loadAllStates()
{
    for (int i = 0; i < m_browserWidgets.size(); ++i) {
        BrowserWidget* widget = m_browserWidgets[i];
        widget->loadState();
    }
}

void WindowManager::setParentWidget(QWidget* parent)
{
    if (m_parentWidget == parent) {
        return;
    }
    
    m_parentWidget = parent;
    setupLayout();
}

QPair<int, int> WindowManager::getGridDimensions(int windowCount)
{
    switch (windowCount) {
        case 0:  return QPair<int, int>(0, 0);
        case 1:  return QPair<int, int>(1, 1);
        case 2:  return QPair<int, int>(1, 2);
        case 3:  return QPair<int, int>(2, 2);  // 2x2 grid with one empty space
        case 4:  return QPair<int, int>(2, 2);
        case 5:  return QPair<int, int>(3, 2);  // 2x3 grid with one empty space
        case 6:  return QPair<int, int>(3, 2);
        case 7:  return QPair<int, int>(4, 2);
        case 8:  return QPair<int, int>(4, 2);  // 2x4 grid
        case 9:  return QPair<int, int>(5, 2);
        case 10: return QPair<int, int>(5, 2);
        case 11: return QPair<int, int>(6, 2);
        case 12: return QPair<int, int>(6, 2);
        case 13: return QPair<int, int>(7, 2);
        case 14: return QPair<int, int>(7, 2);
        case 15: return QPair<int, int>(8, 2);
        case 16: return QPair<int, int>(8, 2);
        default: return QPair<int, int>(0, 0);
    }
}

bool WindowManager::isValidWindowCount(int windowCount)
{
    return SUPPORTED_WINDOW_COUNTS.contains(windowCount);
}

void WindowManager::updateLayout()
{
    if (!m_verticalLayout) {
        qDebug() << "WindowManager::updateLayout: ERROR - m_verticalLayout is null, returning";
        qDebug() << "=== WindowManager::updateLayout END (null layout) ===";
        return;
    }
    
    QLayoutItem* item;
    while ((item = m_verticalLayout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) {
            w->hide();
        }
        delete item;
    }
    
    if (m_currentWindowCount == 0) {
        for (auto w : m_browserWidgets) w->hide();
        return;
    }
    
    int visibleIndex = 0;
    for (int i = 0; i < m_currentWindowCount; i += m_columnCount) {
        QWidget* rowContainer = new QWidget();
        QHBoxLayout* rowLayout = new QHBoxLayout(rowContainer);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(5);
        
        int widgetsInRow = 0;
        for (int j = 0; j < m_columnCount && (i + j) < m_currentWindowCount; ++j) {
            if (visibleIndex < m_browserWidgets.size()) {
                BrowserWidget* widget = m_browserWidgets[visibleIndex];
                
                int widgetWidth = (m_columnCount == 1) ? 880 : 500;
                int widgetHeight = calculateHeightFromWidth(widgetWidth);
                widget->setFixedSize(widgetWidth, widgetHeight);
                
                widget->show();
                rowLayout->addWidget(widget);
                widgetsInRow++;
                visibleIndex++;
            }
        }
        
        rowLayout->addStretch();
        m_verticalLayout->addWidget(rowContainer);
    }
    
    for (int i = m_currentWindowCount; i < m_browserWidgets.size(); ++i) {
        m_browserWidgets[i]->hide();
    }
    
    m_verticalLayout->addStretch();
}

void WindowManager::connectWidgetSignals(BrowserWidget* widget)
{
    if (!widget) return;
    connect(widget, &BrowserWidget::fullscreenRequested, 
            this, &WindowManager::onWidgetFullscreenRequested);
    connect(widget, &BrowserWidget::closeRequested, 
            this, &WindowManager::onWidgetCloseRequested);
}

void WindowManager::disconnectWidgetSignals(BrowserWidget* widget)
{
    if (!widget) return;
    disconnect(widget, &BrowserWidget::fullscreenRequested, 
               this, &WindowManager::onWidgetFullscreenRequested);
    disconnect(widget, &BrowserWidget::closeRequested, 
               this, &WindowManager::onWidgetCloseRequested);
}

void WindowManager::destroyBrowserWidgets()
{
    for (BrowserWidget* widget : m_browserWidgets) {
        if (widget) {
            disconnectWidgetSignals(widget);  // Correct call to disconnect
            widget->deleteLater();
        }
    }
    m_browserWidgets.clear();
}

void WindowManager::onWidgetFullscreenRequested()
{
    BrowserWidget* widget = qobject_cast<BrowserWidget*>(sender());
    if (widget) {
        emit fullscreenRequested(widget);
    } else {
        qDebug() << "WindowManager::onWidgetFullscreenRequested: Failed to cast sender to BrowserWidget";
    }
}

void WindowManager::onWidgetCloseRequested()
{
    BrowserWidget* widget = qobject_cast<BrowserWidget*>(sender());
    if (widget) {
        int index = m_browserWidgets.indexOf(widget);
        if (index >= 0) {
            removeBrowserWidget(index);
        }
    }
}

void WindowManager::onAutoSave()
{
    saveAllStates();
}

void WindowManager::onParentWidgetResized()
{
    // With fixed size layout, widgets don't need to be resized
    // This method is kept for compatibility but does nothing
    return;
}

void WindowManager::forceLayoutUpdate()
{
    if (!m_verticalLayout) {
        return;
    }
    
    // Force the layout to recalculate and update
    m_verticalLayout->invalidate();
    m_verticalLayout->activate();
    
    // Force the parent widget to update its layout
    if (m_parentWidget) {
        m_parentWidget->updateGeometry();
        m_parentWidget->update();
    }
}

void WindowManager::detachWidgetFromLayout(BrowserWidget* widget)
{
    if (!widget) {
        return;
    }
    
    
    // For row-based layout, we need to rebuild the entire layout
    // when detaching widgets to maintain proper row structure
    // But we need to be careful not to destroy the widget we're detaching
    if (m_verticalLayout) {
        // Remove the widget from the layout without destroying it
        m_verticalLayout->removeWidget(widget);
        m_verticalLayout->invalidate();
        m_verticalLayout->activate();
        
        if (m_parentWidget) {
            m_parentWidget->updateGeometry();
            m_parentWidget->update();
        }
    }
}

void WindowManager::attachWidgetToLayout(BrowserWidget* widget, int position)
{
    if (!widget) {
        return;
    }
    
    // Set widget size based on column count
    int widgetWidth = (m_columnCount == 1) ? 880 : 500;
    int widgetHeight = calculateHeightFromWidth(widgetWidth); // 根据5:3比例计算高度
    widget->setFixedSize(widgetWidth, widgetHeight);
    
    // Calculate which row and column this widget should go to
    int row = position / m_columnCount;
    int col = position % m_columnCount;
    
    // Find the correct row container and add the widget to it
    if (m_verticalLayout && row < m_verticalLayout->count()) {
        QLayoutItem* rowItem = m_verticalLayout->itemAt(row);
        if (rowItem && rowItem->widget()) {
            QWidget* rowContainer = rowItem->widget();
            QHBoxLayout* rowLayout = qobject_cast<QHBoxLayout*>(rowContainer->layout());
            if (rowLayout) {
                // Insert the widget at the correct column position
                rowLayout->insertWidget(col, widget);
                
                // Force layout update
                m_verticalLayout->invalidate();
                m_verticalLayout->activate();
                
                if (m_parentWidget) {
                    m_parentWidget->updateGeometry();
                    m_parentWidget->update();
                }

                return;
            }
        }
    }
    
    // If we can't find the correct position, fall back to updateLayout
    updateLayout();
}

void WindowManager::synchronizeWidgetWidths()
{
    // With fixed size layout, all widgets already have consistent sizes
    // No need to synchronize widths
    return;
}

void WindowManager::ensureConsistentWidths()
{
    // With fixed size layout, all widgets already have consistent sizes
    // No need to ensure consistent widths
    return;
}

int WindowManager::calculateDynamicWidth() const
{
    if (!m_parentWidget || !m_gridLayout) {
        return FIXED_WIDGET_WIDTH;
    }
    
    // Calculate width based on parent widget width
    int parentWidth = m_parentWidget->width();
    if (parentWidth <= 0) {
        return FIXED_WIDGET_WIDTH;
    }
    
    // Account for margins and spacing
    int margins = m_gridLayout->contentsMargins().left() + m_gridLayout->contentsMargins().right();
    int spacing = m_gridLayout->spacing();
    
    // Calculate width per column based on column count
    int availableWidth = parentWidth - margins - (spacing * (m_columnCount - 1));
    int widthPerColumn = availableWidth / m_columnCount;
    
    // Ensure minimum width
    return qMax(widthPerColumn, FIXED_WIDGET_WIDTH);
}

void WindowManager::setColumnCount(int columns)
{
    if (columns < 1 || columns > 3) {
        qWarning() << "Invalid column count:" << columns << "Must be 1, 2, or 3";
        return;
    }
    
    m_columnCount = columns;
    
    // Update layout with new column count
    updateLayout();
}

int WindowManager::getColumnCount() const
{
    return m_columnCount;
}

void WindowManager::updateWidgetContent(int index, int subId, const QString& name, const QString& url)
{
    if (index < 0 || index >= m_browserWidgets.size()) {
        qWarning() << "Invalid widget index:" << index;
        return;
    }
    
    BrowserWidget* w = m_browserWidgets[index];
    
    w->setSubWindowId(subId);
    w->setSubWindowName(name);
    
    if (w->isVisible() && !w->getCurrentUrl().isEmpty() && !w->getCurrentUrl().startsWith(url)) {
        w->loadUrl(url);
    } else if (w->isVisible() && w->getCurrentUrl().isEmpty()) {
        w->loadUrl(url);
    }
}

BrowserWidget* WindowManager::findWidgetBySubId(int subId) const
{
    for (BrowserWidget* widget : m_browserWidgets) {
        if (widget && widget->getSubWindowId() == subId) {
            return widget;
        }
    }
    return nullptr;
}

