#include "DatabaseManager.h"
#include <QDir>
#include <QDebug>
#include <QApplication>
#include <QCoreApplication>
#include <QBuffer>
#include <QDataStream>
#include <QIODevice>

DatabaseManager* DatabaseManager::instance = nullptr;

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    close();
}

DatabaseManager* DatabaseManager::getInstance()
{
    if (!instance) {
        instance = new DatabaseManager();
    }
    return instance;
}

bool DatabaseManager::initialize()
{
    QString dbPath = getDatabasePath();

    
    // Create directory if it doesn't exist
    QDir().mkpath(QFileInfo(dbPath).absolutePath());
    
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(dbPath);
    
    if (!database.open()) {
        qDebug() << "Failed to open database:" << database.lastError().text();
        return false;
    }
    
    return createTables();
}

void DatabaseManager::close()
{
    if (database.isOpen()) {
        database.close();
    }
}

QString DatabaseManager::getDatabasePath()
{
    QString executableDir = QCoreApplication::applicationDirPath();
    if (executableDir.isEmpty()) {
        executableDir = QDir::currentPath();
    }

    QDir().mkpath(executableDir);
    return QDir(executableDir).filePath("browser_split_screen.db");
}

bool DatabaseManager::createTables()
{
    return createUsersTable() && 
           createSubWindowsTable() &&
           createWindowConfigsTable() && 
           createHistoryTable() && 
           createBookmarksTable() &&
           createAppSettingsTable() &&
           createUserSessionsTable();
}

bool DatabaseManager::createUsersTable()
{
    QSqlQuery query(database);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_login DATETIME
        )
    )";
    
    if (!query.exec(sql)) {
        qDebug() << "Failed to create users table:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::createSubWindowsTable()
{
    QSqlQuery query(database);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS sub_windows (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            url TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            is_enabled BOOLEAN DEFAULT 1
        )
    )";
    
    if (!query.exec(sql)) {
        qDebug() << "Failed to create sub_windows table:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::createWindowConfigsTable()
{
    QSqlQuery query(database);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS window_configs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            window_id INTEGER UNIQUE NOT NULL,
            sub_id INTEGER DEFAULT -1,
            url TEXT,
            title TEXT,
            geometry TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(sql)) {
        qDebug() << "Failed to create window_configs table:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::createHistoryTable()
{
    QSqlQuery query(database);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            url TEXT NOT NULL,
            title TEXT,
            window_id INTEGER,
            visited_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(sql)) {
        qDebug() << "Failed to create history table:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::createBookmarksTable()
{
    QSqlQuery query(database);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS bookmarks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            url TEXT NOT NULL,
            title TEXT,
            folder TEXT DEFAULT 'Default',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    if (!query.exec(sql)) {
        qDebug() << "Failed to create bookmarks table:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::createAppSettingsTable()
{
    QSqlQuery query(database);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS app_settings (
            key TEXT PRIMARY KEY,
            value BLOB NOT NULL
        )
    )";

    if (!query.exec(sql)) {
        qDebug() << "Failed to create app_settings table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::createUserSessionsTable()
{
    QSqlQuery query(database);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS user_sessions (
            id INTEGER PRIMARY KEY CHECK (id = 1),
            username TEXT,
            remember INTEGER DEFAULT 0,
            last_active DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    if (!query.exec(sql)) {
        qDebug() << "Failed to create user_sessions table:" << query.lastError().text();
        return false;
    }

    return true;
}

QByteArray DatabaseManager::serializeVariant(const QVariant& value) const
{
    QByteArray buffer;
    QBuffer device(&buffer);
    if (!device.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open buffer for writing app setting";
        return QByteArray();
    }

    QDataStream out(&device);
    out.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    out << value;

    return buffer;
}

QVariant DatabaseManager::deserializeVariant(const QByteArray& data, const QVariant& defaultValue) const
{
    if (data.isEmpty()) {
        return defaultValue;
    }

    QBuffer device;
    device.setData(data);
    if (!device.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open buffer for reading app setting";
        return defaultValue;
    }

    QDataStream in(&device);
    in.setVersion(QDataStream::Qt_DefaultCompiledVersion);

    QVariant value;
    in >> value;

    if (!value.isValid()) {
        return defaultValue;
    }

    return value;
}


QString DatabaseManager::hashPassword(const QString& password)
{
    return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
}

bool DatabaseManager::createUser(const QString& username, const QString& password)
{
    if (isUserExists(username)) {
        return false;
    }
    
    QSqlQuery query(database);
    query.prepare("INSERT INTO users (username, password_hash) VALUES (?, ?)");
    query.addBindValue(username);
    query.addBindValue(hashPassword(password));
    
    if (!query.exec()) {
        qDebug() << "Failed to create user:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::authenticateUser(const QString& username, const QString& password)
{
    QSqlQuery query(database);
    query.prepare("SELECT password_hash FROM users WHERE username = ?");
    query.addBindValue(username);
    
    if (!query.exec() || !query.next()) {
        return false;
    }
    
    QString storedHash = query.value(0).toString();
    QString inputHash = hashPassword(password);
    
    if (storedHash == inputHash) {
        // Update last login time
        QSqlQuery updateQuery(database);
        updateQuery.prepare("UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE username = ?");
        updateQuery.addBindValue(username);
        updateQuery.exec();
        return true;
    }
    
    return false;
}

bool DatabaseManager::updateUserPassword(const QString& username, const QString& newPassword)
{
    QSqlQuery query(database);
    query.prepare("UPDATE users SET password_hash = ? WHERE username = ?");
    query.addBindValue(hashPassword(newPassword));
    query.addBindValue(username);
    
    return query.exec();
}

bool DatabaseManager::isUserExists(const QString& username)
{
    QSqlQuery query(database);
    query.prepare("SELECT COUNT(*) FROM users WHERE username = ?");
    query.addBindValue(username);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

bool DatabaseManager::saveUserSession(const QString& username, bool remember)
{
    QSqlQuery query(database);
    query.prepare(R"(
        INSERT INTO user_sessions (id, username, remember, last_active)
        VALUES (1, ?, ?, CURRENT_TIMESTAMP)
        ON CONFLICT(id) DO UPDATE SET
            username = excluded.username,
            remember = excluded.remember,
            last_active = excluded.last_active
    )");
    query.addBindValue(username);
    query.addBindValue(remember ? 1 : 0);

    if (!query.exec()) {
        qDebug() << "Failed to save user session:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::loadUserSession(QString& username, bool& remember)
{
    username.clear();
    remember = false;

    QSqlQuery query(database);
    query.prepare("SELECT username, remember, last_active FROM user_sessions WHERE id = 1");

    if (!query.exec() || !query.next()) {
        return false;
    }

    username = query.value(0).toString();
    remember = query.value(1).toInt() != 0;

    QString timestampString = query.value(2).toString();
    QDateTime timestamp = query.value(2).toDateTime();
    if (!timestamp.isValid()) {
        timestamp = QDateTime::fromString(timestampString, "yyyy-MM-dd HH:mm:ss");
    }
    if (!timestamp.isValid()) {
        timestamp = QDateTime::fromString(timestampString, Qt::ISODate);
    }

    if (!timestamp.isValid()) {
        qDebug() << "loadUserSession: Invalid session timestamp, clearing session";
        clearUserSession();
        username.clear();
        remember = false;
        return false;
    }

    if (timestamp.daysTo(QDateTime::currentDateTime()) > 7) {
        clearUserSession();
        username.clear();
        remember = false;
        return false;
    }

    return !username.isEmpty();
}

void DatabaseManager::clearUserSession()
{
    QSqlQuery query(database);
    query.prepare("DELETE FROM user_sessions WHERE id = 1");

    if (!query.exec()) {
        qDebug() << "Failed to clear user session:" << query.lastError().text();
    }
}

bool DatabaseManager::saveWindowConfig(int windowId, int subId, const QString& url, const QString& title,
                                      const QJsonObject& geometry)
{

    QSqlQuery query(database);
    query.prepare(R"(
        INSERT OR REPLACE INTO window_configs (window_id, sub_id, url, title, geometry, updated_at)
        VALUES (?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
    )");
    query.addBindValue(windowId);
    query.addBindValue(subId);
    query.addBindValue(url);
    query.addBindValue(title);
    query.addBindValue(QJsonDocument(geometry).toJson());

    bool success = query.exec();
    if (!success) {
        qDebug() << "DatabaseManager::saveWindowConfig: Failed to save window config:" << query.lastError().text();
    } else {
    }

    return success;
}

QJsonObject DatabaseManager::loadWindowConfig(int windowId)
{
    QSqlQuery query(database);
    query.prepare("SELECT url, title, geometry FROM window_configs WHERE window_id = ?");
    query.addBindValue(windowId);
    
    QJsonObject config;
    if (query.exec() && query.next()) {
        config["url"] = query.value(0).toString();
        config["title"] = query.value(1).toString();
        
        QJsonDocument doc = QJsonDocument::fromJson(query.value(2).toByteArray());
        if (doc.isObject()) {
            config["geometry"] = doc.object();
        }
    }
    
    return config;
}

bool DatabaseManager::deleteWindowConfig(int windowId)
{
    QSqlQuery query(database);
    query.prepare("DELETE FROM window_configs WHERE window_id = ?");
    query.addBindValue(windowId);
    
    return query.exec();
}

QList<QJsonObject> DatabaseManager::getAllWindowConfigs()
{
    QList<QJsonObject> configs;
    QSqlQuery query(database);
    query.prepare("SELECT window_id, url, title, geometry FROM window_configs ORDER BY window_id");
    
    if (query.exec()) {
        while (query.next()) {
            QJsonObject config;
            config["window_id"] = query.value(0).toInt();
            config["url"] = query.value(1).toString();
            config["title"] = query.value(2).toString();
            
            QJsonDocument doc = QJsonDocument::fromJson(query.value(3).toByteArray());
            if (doc.isObject()) {
                config["geometry"] = doc.object();
            }
            
            configs.append(config);
        }
    }
    
    return configs;
}

bool DatabaseManager::addHistoryRecord(const QString& url, const QString& title, int windowId)
{
    QSqlQuery query(database);
    query.prepare("INSERT INTO history (url, title, window_id) VALUES (?, ?, ?)");
    query.addBindValue(url);
    query.addBindValue(title);
    query.addBindValue(windowId);
    
    return query.exec();
}

QList<QJsonObject> DatabaseManager::getHistoryRecords(int limit)
{
    QList<QJsonObject> records;
    QSqlQuery query(database);
    query.prepare("SELECT url, title, window_id, visited_at FROM history ORDER BY visited_at DESC LIMIT ?");
    query.addBindValue(limit);
    
    if (query.exec()) {
        while (query.next()) {
            QJsonObject record;
            record["url"] = query.value(0).toString();
            record["title"] = query.value(1).toString();
            record["window_id"] = query.value(2).toInt();
            record["visited_at"] = query.value(3).toString();
            
            records.append(record);
        }
    }
    
    return records;
}

bool DatabaseManager::clearHistory()
{
    QSqlQuery query(database);
    return query.exec("DELETE FROM history");
}

bool DatabaseManager::addBookmark(const QString& url, const QString& title, const QString& folder)
{
    QSqlQuery query(database);
    query.prepare("INSERT INTO bookmarks (url, title, folder) VALUES (?, ?, ?)");
    query.addBindValue(url);
    query.addBindValue(title);
    query.addBindValue(folder);
    
    return query.exec();
}

bool DatabaseManager::removeBookmark(const QString& url)
{
    QSqlQuery query(database);
    query.prepare("DELETE FROM bookmarks WHERE url = ?");
    query.addBindValue(url);
    
    return query.exec();
}

QList<QJsonObject> DatabaseManager::getBookmarks(const QString& folder)
{
    QList<QJsonObject> bookmarks;
    QSqlQuery query(database);
    
    if (folder.isEmpty()) {
        query.prepare("SELECT url, title, folder, created_at FROM bookmarks ORDER BY created_at DESC");
    } else {
        query.prepare("SELECT url, title, folder, created_at FROM bookmarks WHERE folder = ? ORDER BY created_at DESC");
        query.addBindValue(folder);
    }
    
    if (query.exec()) {
        while (query.next()) {
            QJsonObject bookmark;
            bookmark["url"] = query.value(0).toString();
            bookmark["title"] = query.value(1).toString();
            bookmark["folder"] = query.value(2).toString();
            bookmark["created_at"] = query.value(3).toString();
            
            bookmarks.append(bookmark);
        }
    }
    
    return bookmarks;
}

bool DatabaseManager::updateBookmark(const QString& url, const QString& newTitle, const QString& newFolder)
{
    QSqlQuery query(database);
    query.prepare("UPDATE bookmarks SET title = ?, folder = ? WHERE url = ?");
    query.addBindValue(newTitle);
    query.addBindValue(newFolder);
    query.addBindValue(url);
    
    return query.exec();
}

bool DatabaseManager::setAppSetting(const QString& key, const QVariant& value)
{
    QByteArray serializedValue = serializeVariant(value);

    QSqlQuery query(database);
    query.prepare(R"(
        INSERT INTO app_settings (key, value)
        VALUES (?, ?)
        ON CONFLICT(key) DO UPDATE SET value = excluded.value
    )");
    query.addBindValue(key);
    query.addBindValue(serializedValue);

    if (!query.exec()) {
        qDebug() << "Failed to persist app setting for key" << key << ":" << query.lastError().text();
        return false;
    }

    return true;
}

QVariant DatabaseManager::getAppSetting(const QString& key, const QVariant& defaultValue)
{
    QSqlQuery query(database);
    query.prepare("SELECT value FROM app_settings WHERE key = ?");
    query.addBindValue(key);

    if (query.exec() && query.next()) {
        QByteArray storedValue = query.value(0).toByteArray();
        return deserializeVariant(storedValue, defaultValue);
    }

    return defaultValue;
}

bool DatabaseManager::removeAppSetting(const QString& key)
{
    QSqlQuery query(database);
    query.prepare("DELETE FROM app_settings WHERE key = ?");
    query.addBindValue(key);

    if (!query.exec()) {
        qDebug() << "Failed to remove app setting for key" << key << ":" << query.lastError().text();
        return false;
    }

    return true;
}

// SubWindow management methods
bool DatabaseManager::addSubWindow(const QString& name, const QString& url)
{
    QSqlQuery query(database);
    query.prepare("INSERT INTO sub_windows (name, url) VALUES (?, ?)");
    query.addBindValue(name);
    query.addBindValue(url);
    
    if (!query.exec()) {
        qDebug() << "Failed to add sub window:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::updateSubWindow(int subWindowId, const QString& name, const QString& url)
{
    QSqlQuery query(database);
    query.prepare("UPDATE sub_windows SET name = ?, url = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?");
    query.addBindValue(name);
    query.addBindValue(url);
    query.addBindValue(subWindowId);
    
    if (!query.exec()) {
        qDebug() << "Failed to update sub window:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

bool DatabaseManager::deleteSubWindow(int subWindowId)
{
    QSqlQuery query(database);
    query.prepare("DELETE FROM sub_windows WHERE id = ?");
    query.addBindValue(subWindowId);
    
    if (!query.exec()) {
        qDebug() << "Failed to delete sub window:" << query.lastError().text();
        return false;
    }
    
    return query.numRowsAffected() > 0;
}

QList<QJsonObject> DatabaseManager::getAllSubWindows()
{
    QList<QJsonObject> subWindows;
    QSqlQuery query(database);
    query.prepare("SELECT id, name, url, created_at, updated_at, is_enabled FROM sub_windows ORDER BY created_at DESC");
    
    
    if (query.exec()) {
        while (query.next()) {
            QJsonObject subWindow;
            subWindow["id"] = query.value(0).toInt();
            subWindow["name"] = query.value(1).toString();
            subWindow["url"] = query.value(2).toString();
            subWindow["created_at"] = query.value(3).toString();
            subWindow["updated_at"] = query.value(4).toString();
            subWindow["is_enabled"] = query.value(5).toBool();
            
            subWindows.append(subWindow);
        }
    } else {
        qDebug() << "DatabaseManager::getAllSubWindows: Query failed:" << query.lastError().text();
    }
    
    return subWindows;
}

QJsonObject DatabaseManager::getSubWindow(int subWindowId)
{
    QJsonObject subWindow;
    QSqlQuery query(database);
    query.prepare("SELECT id, name, url, created_at, updated_at, is_enabled FROM sub_windows WHERE id = ?");
    query.addBindValue(subWindowId);
    
    if (query.exec() && query.next()) {
        subWindow["id"] = query.value(0).toInt();
        subWindow["name"] = query.value(1).toString();
        subWindow["url"] = query.value(2).toString();
        subWindow["created_at"] = query.value(3).toString();
        subWindow["updated_at"] = query.value(4).toString();
        subWindow["is_enabled"] = query.value(5).toBool();
    }
    
    return subWindow;
}

bool DatabaseManager::deleteWindowConfigsBySubId(int subId)
{
    QSqlQuery query(database);
    query.prepare("DELETE FROM window_configs WHERE sub_id = ?");
    query.addBindValue(subId);
    
    if (!query.exec()) {
        qDebug() << "Failed to delete window configs by subId:" << query.lastError().text();
        return false;
    }
    
    return true;
}
