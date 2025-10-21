#ifndef SUBWINDOWMANAGER_H
#define SUBWINDOWMANAGER_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QJsonObject>

class SubWindowManager : public QDialog
{
    Q_OBJECT

public:
    explicit SubWindowManager(QWidget *parent = nullptr);
    ~SubWindowManager();

    void refreshSubWindows();
    QList<QJsonObject> getSubWindows() const;

signals:
    void subWindowAdded(const QJsonObject& subWindow);
    void subWindowRefreshRequested(int subId);
    void subWindowUpdated(const QJsonObject& subWindow);
    void subWindowDeleted(int subWindowId);

private slots:
    void onAddSubWindow();
    void onDeleteSubWindow();
    void onSubWindowSelectionChanged();
    void onCheckboxChanged(QTableWidgetItem* item);
    void onUrlChanged(QTableWidgetItem* item);

private:
    void setupUI();
    void setupConnections();
    void loadSubWindows();
    void addSubWindowToTable(const QJsonObject& subWindow);
    void updateSubWindowInTable(int row, const QJsonObject& subWindow);
    bool validateSubWindowData(const QString& name, const QString& url);
    QJsonObject getSubWindowFromTable(int row);

    // UI Components
    QTableWidget* m_tableWidget;
    QPushButton* m_addButton;
    QPushButton* m_deleteButton;
    QPushButton* m_refreshButton;
    QDialogButtonBox* m_buttonBox;

    // Data
    QList<QJsonObject> m_subWindows;
};

// SubWindow Edit Dialog
class SubWindowEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubWindowEditDialog(const QJsonObject& subWindow = QJsonObject(), QWidget *parent = nullptr);
    ~SubWindowEditDialog();

    QJsonObject getSubWindowData() const;
    void setSubWindowData(const QJsonObject& subWindow);

private slots:
    void onUrlChanged();
    void validateInput();

private:
    void setupUI();
    void setupConnections();
    bool validateUrl(const QString& url);

    // UI Components
    QLineEdit* m_nameEdit;
    QLineEdit* m_urlEdit;
    QLabel* m_urlStatusLabel;
    QDialogButtonBox* m_buttonBox;

    // Data
    QJsonObject m_subWindowData;
};

#endif // SUBWINDOWMANAGER_H
