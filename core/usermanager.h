#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>

class UserManager : public QObject
{
    Q_OBJECT
public:
    enum UserRole {
        Guest,
        Operator,
        Admin
    };
    Q_ENUM(UserRole)

    struct User {
        QString username;
        UserRole role;
    };

    static UserManager& instance();

    // 登录
    bool login(const QString& username, const QString& password);
    
    // 注销
    void logout();

    // 获取当前用户
    User currentUser() const;
    
    // 检查是否有特定权限
    bool isAdmin() const;

    // 获取角色名称
    static QString roleName(UserRole role);

    // --- 用户管理接口 ---
    QList<User> getAllUsers() const;
    bool addUser(const QString& username, const QString& password, UserRole role);
    bool updateUser(const QString& username, const QString& password, UserRole role);
    bool removeUser(const QString& username);

signals:
    void userChanged(const User& user);
    void loginFailed(const QString& reason);

private:
    UserManager(QObject* parent = nullptr);
    ~UserManager() = default;
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    User m_currentUser;
    
    // 模拟的用户数据库: username -> {password, role}
    struct UserData {
        QString password;
        UserRole role;
    };
    QMap<QString, UserData> m_users;

    void loadUsers();
    void saveUsers();
};

#endif // USERMANAGER_H
