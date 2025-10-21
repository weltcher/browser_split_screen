#include "SubWindowManager.h"
#include "DatabaseManager.h"
#include <QHeaderView>
#include <QUrl>
#include <QRegularExpression>
#include <QDateTime>
#include <QDebug> // Added for qDebug
#include <QMessageBox> // Added for QMessageBox
#include <QVBoxLayout> // Added for QVBoxLayout
#include <QHBoxLayout> // Added for QHBoxLayout
#include <QFormLayout> // Added for QFormLayout
#include <QLineEdit> // Added for QLineEdit
#include <QLabel> // Added for QLabel
#include <QDialogButtonBox> // Added for QDialogButtonBox
#include <QJsonObject> // Added for QJsonObject
#include <QList> // Added for QList
#include <QJsonArray> // Added for QJsonArray
#include <QTimer> // Added for delayed updates

SubWindowManager::SubWindowManager(QWidget *parent)
    : QDialog(parent)
    , m_tableWidget(nullptr)
    , m_addButton(nullptr)
    , m_deleteButton(nullptr)
    , m_refreshButton(nullptr)
    , m_buttonBox(nullptr)
{
    setupUI();
    setupConnections();
    loadSubWindows();
}

SubWindowManager::~SubWindowManager()
{
}

void SubWindowManager::setupUI()
{
    setWindowTitle("子窗口管理");
    setModal(true);
    resize(800, 600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Table widget
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(5);
    m_tableWidget->setHorizontalHeaderLabels({"选择", "ID", "名称", "网址", "创建时间"});
    
    // Set table properties
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::NoSelection);  // Disable built-in selection since using checkboxes
    
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->setSortingEnabled(true);
    
    // Set column widths
    QHeaderView* header = m_tableWidget->horizontalHeader();
    header->setStretchLastSection(true);
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // Checkbox column fixed
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // ID
    header->setSectionResizeMode(2, QHeaderView::Stretch);  // Name
    header->setSectionResizeMode(3, QHeaderView::Stretch);  // URL
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);  // Created at

    // 设置表头居中
    header->setDefaultAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    // 设置表格内容居中
    // 样式表方式对齐不生效，我们直接为每一项设置对齐
    // 在填充/刷新表格时(for每个item)，应调用 item->setTextAlignment(Qt::AlignCenter)
    // 这里只做一遍所有已有行列的内容居中（比如初始化时表为空），实际填充数据时也要加
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
            QTableWidgetItem* item = m_tableWidget->item(row, col);
            if (item) {
                item->setTextAlignment(Qt::AlignCenter);
            }
        }
    }
    
    mainLayout->addWidget(m_tableWidget);
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_addButton = new QPushButton("添加子窗口", this);
    m_addButton->setIcon(QIcon(":/icons/add.png"));
    
    m_deleteButton = new QPushButton("删除", this);
    m_deleteButton->setIcon(QIcon(":/icons/delete.png"));
    m_deleteButton->setEnabled(false);
    
    m_refreshButton = new QPushButton("刷新", this);
    m_refreshButton->setIcon(QIcon(":/icons/refresh.png"));
    
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_refreshButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Dialog buttons
    // m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    // mainLayout->addWidget(m_buttonBox);
}

void SubWindowManager::setupConnections()
{
    connect(m_addButton, &QPushButton::clicked, this, &SubWindowManager::onAddSubWindow);
    connect(m_deleteButton, &QPushButton::clicked, this, &SubWindowManager::onDeleteSubWindow);
    connect(m_refreshButton, &QPushButton::clicked, this, &SubWindowManager::refreshSubWindows);
    
    // Checkbox changes
    connect(m_tableWidget, &QTableWidget::itemChanged, this, &SubWindowManager::onCheckboxChanged);
    connect(m_tableWidget, &QTableWidget::itemChanged, this, &SubWindowManager::onUrlChanged);
    
    connect(m_tableWidget, &QTableWidget::itemSelectionChanged, 
            this, &SubWindowManager::onSubWindowSelectionChanged);
}

void SubWindowManager::refreshSubWindows()
{
    loadSubWindows();
}

QList<QJsonObject> SubWindowManager::getSubWindows() const
{
    return m_subWindows;
}

void SubWindowManager::loadSubWindows()
{
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    if (!dbManager) {
        qWarning() << "SubWindowManager::loadSubWindows: DatabaseManager is null, cannot load subwindows";
        m_subWindows.clear();
        m_tableWidget->setRowCount(0);
        qDebug() << "=== SubWindowManager::loadSubWindows END (DB null) ===";
        return;
    }
    m_subWindows = dbManager->getAllSubWindows();
    
    m_tableWidget->setRowCount(m_subWindows.size());
    
    for (int i = 0; i < m_subWindows.size(); ++i) {
        QJsonObject subWindow = m_subWindows[i];
        updateSubWindowInTable(i, subWindow);
    }
    
    // Delayed finalization to avoid signal conflicts during init
    QTimer::singleShot(0, this, [this]() {
        onSubWindowSelectionChanged();
    });
    
}

void SubWindowManager::addSubWindowToTable(const QJsonObject& subWindow)
{
    int row = m_tableWidget->rowCount();
    m_tableWidget->insertRow(row);
    updateSubWindowInTable(row, subWindow);
}

void SubWindowManager::updateSubWindowInTable(int row, const QJsonObject& subWindow)
{
    
    if (row < 0 || row >= m_tableWidget->rowCount()) {
        qWarning() << "Invalid row" << row << "in updateSubWindowInTable";
        return;
    }
    
    // Checkbox column (0)
    QTableWidgetItem* checkItem = new QTableWidgetItem();
    checkItem->setCheckState(Qt::Unchecked);
    checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    checkItem->setTextAlignment(Qt::AlignCenter);
    m_tableWidget->setItem(row, 0, checkItem);
    
    // ID (column 1)
    QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(subWindow.value("id").toInt(-1)));
    idItem->setTextAlignment(Qt::AlignCenter);
    idItem->setFlags(Qt::ItemIsEnabled);  // Read-only
    m_tableWidget->setItem(row, 1, idItem);
    
    // Name (column 2)
    QTableWidgetItem* nameItem = new QTableWidgetItem(subWindow.value("name").toString());
    nameItem->setTextAlignment(Qt::AlignCenter);
    nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
    m_tableWidget->setItem(row, 2, nameItem);
    
    // URL (column 3)
    QTableWidgetItem* urlItem = new QTableWidgetItem(subWindow.value("url").toString());
    urlItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);  // URLs left-aligned
    urlItem->setFlags(urlItem->flags() | Qt::ItemIsEditable);
    m_tableWidget->setItem(row, 3, urlItem);
    
    // Created at (column 4) - Safe handling: Skip or set 'N/A' if missing/empty
    QString createdAt = subWindow.value("created_at").toString();
    if (createdAt.isEmpty() || !subWindow.contains("created_at")) {
        createdAt = "N/A";  // Fallback to avoid empty item issues
        qWarning() << "  Created_at missing/empty for row" << row << ", setting to 'N/A'";
    } else {
    }
    QTableWidgetItem* createdItem = new QTableWidgetItem(createdAt);
    createdItem->setTextAlignment(Qt::AlignCenter);
    createdItem->setFlags(Qt::ItemIsEnabled);  // Read-only, set after creation
    m_tableWidget->setItem(row, 4, createdItem);
    
}

bool SubWindowManager::validateSubWindowData(const QString& name, const QString& url)
{
    if (name.trimmed().isEmpty()) {
        QMessageBox::warning(this, "验证错误", "子窗口名称不能为空");
        return false;
    }
    
    if (url.trimmed().isEmpty()) {
        QMessageBox::warning(this, "验证错误", "网址不能为空");
        return false;
    }
    
    // Basic URL validation
    QUrl qurl(url);
    if (!qurl.isValid() || qurl.scheme().isEmpty()) {
        QMessageBox::warning(this, "验证错误", "请输入有效的网址（如：https://www.example.com）");
        return false;
    }
    
    return true;
}

QJsonObject SubWindowManager::getSubWindowFromTable(int row)
{
    QJsonObject subWindow;
    subWindow["id"] = m_tableWidget->item(row, 1)->text().toInt();  // Skip checkbox
    subWindow["name"] = m_tableWidget->item(row, 2)->text();
    subWindow["url"] = m_tableWidget->item(row, 3)->text();
    subWindow["created_at"] = m_tableWidget->item(row, 4)->text();
    return subWindow;
}

void SubWindowManager::onAddSubWindow()
{
    SubWindowEditDialog dialog(QJsonObject(), this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject subWindowData = dialog.getSubWindowData();
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        if (!dbManager) {
            qWarning() << "SubWindowManager::onAddSubWindow: DatabaseManager is null, cannot add subwindow";
            QMessageBox::critical(this, "错误", "数据库未初始化，无法添加子窗口");
            return;
        }
        if (dbManager->addSubWindow(subWindowData["name"].toString(), 
                                   subWindowData["url"].toString())) {
            // Get the newly added sub window with ID from database
            QList<QJsonObject> allSubWindows = dbManager->getAllSubWindows();
            QJsonObject newSubWindow;
            for (const QJsonObject& subWindow : allSubWindows) {
                if (subWindow["name"].toString() == subWindowData["name"].toString() &&
                    subWindow["url"].toString() == subWindowData["url"].toString()) {
                    newSubWindow = subWindow;
                    break;
                }
            }
            

            // Save window_config for the new subwindow
            if (!newSubWindow.isEmpty()) {
                int newSubId = newSubWindow["id"].toInt();
                QString name = newSubWindow["name"].toString();
                QString url = newSubWindow["url"].toString();

                // Use subId as window_id for window_configs (1:1 mapping with sub_windows)
                QJsonObject geometry;
                geometry["x"] = 0;
                geometry["y"] = 0;
                geometry["width"] = 500;
                geometry["height"] = 300;

                if (dbManager->saveWindowConfig(newSubId, newSubId, url, name, geometry)) {
                } else {
                    qWarning() << "SubWindowManager::onAddSubWindow: Failed to save window_config for subId:" << newSubId;
                }
            }

            refreshSubWindows();
            emit subWindowAdded(newSubWindow);
            // QMessageBox::information(this, "成功", "子窗口添加成功");
        } else {
            QMessageBox::critical(this, "错误", "添加子窗口失败");
        }
    }
}

void SubWindowManager::onDeleteSubWindow()
{
    // Get all checked rows from checkbox column
    QList<int> checkedRows;
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        QTableWidgetItem* checkItem = m_tableWidget->item(row, 0);
        if (checkItem && checkItem->checkState() == Qt::Checked) {
            checkedRows.append(row);
        }
    }
    
    if (checkedRows.isEmpty()) return;
    
    int numSelected = checkedRows.size();
    QString names;
    for (int row : checkedRows) {
        if (row >= 0 && row < m_tableWidget->rowCount()) {
            names += m_tableWidget->item(row, 2)->text() + ", ";  // Name in column 2
        }
    }
    if (!names.isEmpty()) {
        names.chop(2);  // Remove trailing ", "
    }
    
    int ret = QMessageBox::question(this, "确认批量删除", 
                                   QString("确定要删除 %1 个子窗口吗？\n\n子窗口: %2").arg(numSelected).arg(names),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        DatabaseManager* dbManager = DatabaseManager::getInstance();
        if (!dbManager) {
            qWarning() << "SubWindowManager::onDeleteSubWindow: DatabaseManager is null, cannot delete subwindows";
            QMessageBox::critical(this, "错误", "数据库未初始化，无法删除子窗口");
            return;
        }
        bool allSuccess = true;
        
        // Sort rows descending to avoid index issues
        std::sort(checkedRows.begin(), checkedRows.end(), std::greater<int>());
        
        for (int row : checkedRows) {
            if (row >= 0 && row < m_tableWidget->rowCount()) {
                QJsonObject subWindowData = getSubWindowFromTable(row);
                int id = subWindowData["id"].toInt();
                if (dbManager->deleteSubWindow(id)) {
                    emit subWindowDeleted(id);
                } else {
                    allSuccess = false;
                }
            }
        }
        
        refreshSubWindows();
        
        QString message = allSuccess ? "批量删除成功" : "部分删除失败";
        QMessageBox::information(this, "操作结果", message);
    }
}

void SubWindowManager::onSubWindowSelectionChanged()
{
    // For delete: enable if any checkbox is checked
    int checkedCount = 0;
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        QTableWidgetItem* checkItem = m_tableWidget->item(row, 0);
        if (checkItem && checkItem->checkState() == Qt::Checked) {
            checkedCount++;
        }
    }
    m_deleteButton->setEnabled(checkedCount > 0);
}

void SubWindowManager::onCheckboxChanged(QTableWidgetItem* item)
{
    if (item->column() == 0) {  // Checkbox column
        // Trigger selection change to update buttons
        onSubWindowSelectionChanged();
    }
}

// Add new slot after onCheckboxChanged
void SubWindowManager::onUrlChanged(QTableWidgetItem* item)
{
    static bool isUpdating = false;
    if (isUpdating) {
        return;
    }
    isUpdating = true;
    
    if (!item || item->column() != 3) {  // Only handle URL column (3)
        isUpdating = false;
        return;
    }
    
    int row = item->row();
    if (row < 0 || row >= m_tableWidget->rowCount()) {
        isUpdating = false;
        return;
    }
    
    QString newUrl = item->text().trimmed();
    if (newUrl.isEmpty()) {
        QMessageBox::warning(this, "无效网址", "网址不能为空");
        // Revert to original or refresh
        refreshSubWindows();
        isUpdating = false;
        return;
    }
    
    // Basic URL validation
    QUrl qurl(newUrl);
    if (!qurl.isValid() || qurl.scheme().isEmpty()) {
        QMessageBox::warning(this, "无效网址", "请输入有效的网址（如：https://www.example.com）");
        // Revert
        refreshSubWindows();
        isUpdating = false;
        return;
    }
    
    int id = m_tableWidget->item(row, 1)->text().toInt();
    QString name = m_tableWidget->item(row, 2)->text();
    
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    if (!dbManager) {
        qWarning() << "SubWindowManager::onUrlChanged: DatabaseManager is null, cannot update subwindow";
        QMessageBox::critical(this, "错误", "数据库未初始化，无法更新子窗口");
        refreshSubWindows();
        isUpdating = false;
        return;
    }
    
    if (dbManager->updateSubWindow(id, name, newUrl)) {
        // Create updated JSON object
        QJsonObject updatedData;
        updatedData["id"] = id;
        updatedData["name"] = name;
        updatedData["url"] = newUrl;
        // Add other fields if needed, e.g., created_at from table
        if (m_tableWidget->item(row, 4)) {
            updatedData["created_at"] = m_tableWidget->item(row, 4)->text();
        }
        
        emit subWindowUpdated(updatedData);
        // Removed the success popup as per user request
    } else {
        QMessageBox::critical(this, "错误", "更新网址失败");
        refreshSubWindows();  // Revert
    }
    isUpdating = false;
}

// SubWindowEditDialog Implementation
SubWindowEditDialog::SubWindowEditDialog(const QJsonObject& subWindow, QWidget *parent)
    : QDialog(parent)
    , m_nameEdit(nullptr)
    , m_urlEdit(nullptr)
    , m_urlStatusLabel(nullptr)
    , m_buttonBox(nullptr)
    , m_subWindowData(subWindow)
{
    setupUI();
    setupConnections();
    
    if (!subWindow.isEmpty()) {
        setSubWindowData(subWindow);
    }
}

SubWindowEditDialog::~SubWindowEditDialog()
{
}

void SubWindowEditDialog::setupUI()
{
    setWindowTitle(m_subWindowData.isEmpty() ? "添加子窗口" : "编辑子窗口");
    setModal(true);
    resize(400, 200);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QFormLayout* formLayout = new QFormLayout();
    
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("请输入子窗口名称");
    formLayout->addRow("子窗口名称:", m_nameEdit);
    
    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText("请输入网址，如：https://www.example.com");
    formLayout->addRow("网址:", m_urlEdit);
    
    m_urlStatusLabel = new QLabel(this);
    m_urlStatusLabel->setStyleSheet("color: red; font-size: 10px;");
    formLayout->addRow("", m_urlStatusLabel);
    
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
    
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);
}

void SubWindowEditDialog::setupConnections()
{
    connect(m_urlEdit, &QLineEdit::textChanged, this, &SubWindowEditDialog::onUrlChanged);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &SubWindowEditDialog::validateInput);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    // Initial validation
    validateInput();
}

QJsonObject SubWindowEditDialog::getSubWindowData() const
{
    QJsonObject data;
    data["name"] = m_nameEdit->text().trimmed();
    data["url"] = m_urlEdit->text().trimmed();
    return data;
}

void SubWindowEditDialog::setSubWindowData(const QJsonObject& subWindow)
{
    m_nameEdit->setText(subWindow["name"].toString());
    m_urlEdit->setText(subWindow["url"].toString());
}

void SubWindowEditDialog::onUrlChanged()
{
    QString url = m_urlEdit->text().trimmed();
    
    if (url.isEmpty()) {
        m_urlStatusLabel->setText("");
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }
    
    if (validateUrl(url)) {
        m_urlStatusLabel->setText("✓ 网址格式正确");
        m_urlStatusLabel->setStyleSheet("color: green; font-size: 10px;");
    } else {
        m_urlStatusLabel->setText("✗ 网址格式不正确");
        m_urlStatusLabel->setStyleSheet("color: red; font-size: 10px;");
    }
    
    validateInput();
}

void SubWindowEditDialog::validateInput()
{
    bool isValid = !m_nameEdit->text().trimmed().isEmpty() && 
                   !m_urlEdit->text().trimmed().isEmpty() && 
                   validateUrl(m_urlEdit->text().trimmed());
    
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid);
}

bool SubWindowEditDialog::validateUrl(const QString& url)
{
    if (url.isEmpty()) return false;
    
    QUrl qurl(url);
    if (!qurl.isValid()) return false;
    
    // Check if scheme is present, if not, try adding http://
    if (qurl.scheme().isEmpty()) {
        qurl = QUrl("http://" + url);
        if (!qurl.isValid()) return false;
    }
    
    return true;
}
