#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QHash>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QVariant>
#include <QByteArray>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager* getInstance();
    
    bool initialize();
    void close();
    
    // User management
    bool createUser(const QString& username, const QString& password);
    bool authenticateUser(const QString& username, const QString& password);
    bool updateUserPassword(const QString& username, const QString& newPassword);
    bool isUserExists(const QString& username);
    bool saveUserSession(const QString& username, bool remember);
    bool loadUserSession(QString& username, bool& remember);
    void clearUserSession();
    
    // SubWindow management
    bool addSubWindow(const QString& name, const QString& url);
    bool updateSubWindow(int subWindowId, const QString& name, const QString& url);
    bool deleteSubWindow(int subWindowId);
    QList<QJsonObject> getAllSubWindows();
    QJsonObject getSubWindow(int subWindowId);
    
    
    // Window management
    bool saveWindowConfig(int windowId, int subId, const QString& url, const QString& title, 
                          const QJsonObject& geometry);
    QJsonObject loadWindowConfig(int windowId);
    bool deleteWindowConfig(int windowId);
    QList<QJsonObject> getAllWindowConfigs();
    bool deleteWindowConfigsBySubId(int subId);
    
    // History management
    bool addHistoryRecord(const QString& url, const QString& title, int windowId);
    QList<QJsonObject> getHistoryRecords(int limit = 100);
    bool clearHistory();
    
    // Bookmarks management
    bool addBookmark(const QString& url, const QString& title, const QString& folder = "Default");
    bool removeBookmark(const QString& url);
    QList<QJsonObject> getBookmarks(const QString& folder = "");
    bool updateBookmark(const QString& url, const QString& newTitle, const QString& newFolder);

    // Application settings
    bool setAppSetting(const QString& key, const QVariant& value);
    QVariant getAppSetting(const QString& key, const QVariant& defaultValue = QVariant());
    bool removeAppSetting(const QString& key);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();
    
    static DatabaseManager* instance;
    QSqlDatabase database;
    
    bool createTables();
    QString hashPassword(const QString& password);
    QString getDatabasePath();
    
    // Table creation methods
    bool createUsersTable();
    bool createSubWindowsTable();
    bool createWindowConfigsTable();
    bool createHistoryTable();
    bool createBookmarksTable();
    bool createAppSettingsTable();
    bool createUserSessionsTable();

    QByteArray serializeVariant(const QVariant& value) const;
    QVariant deserializeVariant(const QByteArray& data, const QVariant& defaultValue) const;
};

#endif // DATABASEMANAGER_H
