#include "configmanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() 
    : m_settings("EddyPusher", "Config")
{
}

// --- 串口配置 ---
int ConfigManager::serialBaudRate() const 
{ 
    return m_settings.value("Serial/BaudRate", 115200).toInt(); 
}

void ConfigManager::setSerialBaudRate(int baud) 
{ 
    m_settings.setValue("Serial/BaudRate", baud); 
}

// --- 运动保护配置 ---
double ConfigManager::maxSpeed() const 
{ 
    return m_settings.value("Motion/MaxSpeed", 100.0).toDouble(); 
}

void ConfigManager::setMaxSpeed(double speed) 
{ 
    m_settings.setValue("Motion/MaxSpeed", speed); 
}

double ConfigManager::maxPosition() const 
{ 
    return m_settings.value("Motion/MaxPosition", 1000.0).toDouble(); 
}

void ConfigManager::setMaxPosition(double pos) 
{ 
    m_settings.setValue("Motion/MaxPosition", pos); 
}

int ConfigManager::motionTimeout() const 
{ 
    return m_settings.value("Motion/TimeoutMs", 30000).toInt(); 
}

void ConfigManager::setMotionTimeout(int ms) 
{ 
    m_settings.setValue("Motion/TimeoutMs", ms); 
}

// --- 数据存储配置 ---
QString ConfigManager::dataStoragePath() const 
{
    // 强制使用程序当前目录下的 AppData 文件夹（避免与源代码的 data 文件夹冲突）
    // 不再从配置文件读取，避免使用旧的系统路径
    QString appDirPath = QCoreApplication::applicationDirPath() + "/AppData";
    
    // 检查配置文件中是否有旧路径，如果有则更新为新路径
    QString savedPath = m_settings.value("Data/StoragePath", "").toString();
    if (savedPath != appDirPath && !savedPath.isEmpty()) {
        // 更新配置文件中的路径
        const_cast<QSettings&>(m_settings).setValue("Data/StoragePath", appDirPath);
    }
    
    return appDirPath;
}

void ConfigManager::setDataStoragePath(const QString &path) 
{ 
    m_settings.setValue("Data/StoragePath", path); 
}

// 辅助: 确保数据目录存在
void ConfigManager::ensureDataDirExists() 
{
    QDir dir(dataStoragePath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}
