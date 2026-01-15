#include "usermanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

UserManager& UserManager::instance()
{
    static UserManager instance;
    return instance;
}

UserManager::UserManager(QObject* parent) : QObject(parent)
{
    // 初始化默认用户
    m_currentUser = {"", Guest};

    loadUsers();

    // 如果没有任何用户（第一次运行），添加默认管理员
    if (m_users.isEmpty()) {
        m_users.insert("admin", {"123456", Admin});
        m_users.insert("op",    {"123",    Operator});
        saveUsers();
    }
}

bool UserManager::login(const QString& username, const QString& password)
{
    if (!m_users.contains(username)) {
        emit loginFailed("用户名不存在");
        return false;
    }

    const UserData& data = m_users[username];
    if (data.password != password) {
        emit loginFailed("密码错误");
        return false;
    }

    m_currentUser = {username, data.role};
    emit userChanged(m_currentUser);
    return true;
}

void UserManager::logout()
{
    m_currentUser = {"", Guest};
    emit userChanged(m_currentUser);
}

UserManager::User UserManager::currentUser() const
{
    return m_currentUser;
}

bool UserManager::isAdmin() const
{
    return m_currentUser.role == Admin;
}

QString UserManager::roleName(UserRole role)
{
    switch (role) {
    case Admin: return "管理员";
    case Operator: return "操作员";
    default: return "访客";
    }
}

QList<UserManager::User> UserManager::getAllUsers() const
{
    QList<User> list;
    for (auto it = m_users.begin(); it != m_users.end(); ++it) {
        list.append({it.key(), it.value().role});
    }
    return list;
}

bool UserManager::addUser(const QString& username, const QString& password, UserRole role)
{
    if (username.isEmpty() || password.isEmpty()) return false;
    if (m_users.contains(username)) return false;

    m_users.insert(username, {password, role});
    saveUsers();
    return true;
}

bool UserManager::updateUser(const QString& username, const QString& password, UserRole role)
{
    if (!m_users.contains(username)) return false;

    UserData data = m_users[username];
    data.role = role;
    if (!password.isEmpty()) { // 如果密码为空则不修改
        data.password = password;
    }
    m_users[username] = data;
    saveUsers();
    return true;
}

bool UserManager::removeUser(const QString& username)
{
    if (!m_users.contains(username)) return false;
    // 不允许删除最后一个管理员，防止死锁
    if (m_users[username].role == Admin) {
        int adminCount = 0;
        for (const auto& u : m_users) {
            if (u.role == Admin) adminCount++;
        }
        if (adminCount <= 1) return false;
    }
    
    m_users.remove(username);
    saveUsers();
    return true;
}

void UserManager::loadUsers()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/users.json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const auto& val : arr) {
            QJsonObject obj = val.toObject();
            QString u = obj["username"].toString();
            QString p = obj["password"].toString();
            int r = obj["role"].toInt();
            if (!u.isEmpty()) {
                m_users.insert(u, {p, static_cast<UserRole>(r)});
            }
        }
    }
}

void UserManager::saveUsers()
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dirPath);
    if (!dir.exists()) dir.mkpath(".");

    QString path = dirPath + "/users.json";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save users to" << path;
        return;
    }

    QJsonArray arr;
    for (auto it = m_users.begin(); it != m_users.end(); ++it) {
        QJsonObject obj;
        obj["username"] = it.key();
        obj["password"] = it.value().password;
        obj["role"] = static_cast<int>(it.value().role);
        arr.append(obj);
    }

    file.write(QJsonDocument(arr).toJson());
}
