#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>

/**
 * @brief 配置管理类 (单例模式)
 * 负责应用程序的参数存储和读取
 */
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    static ConfigManager& instance();

    // --- 串口配置 ---
    int serialBaudRate() const;
    void setSerialBaudRate(int baud);

    // --- 运动保护配置 ---
    double maxSpeed() const;
    void setMaxSpeed(double speed);

    double maxPosition() const;
    void setMaxPosition(double pos);

    int motionTimeout() const;
    void setMotionTimeout(int ms);

    // --- 数据存储配置 ---
    QString dataStoragePath() const;
    void setDataStoragePath(const QString &path);

    // 辅助: 确保数据目录存在
    void ensureDataDirExists();

private:
    ConfigManager();
    QSettings m_settings;
};

#endif // CONFIGMANAGER_H
