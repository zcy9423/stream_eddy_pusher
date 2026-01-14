#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include "../communication/protocol.h"

/**
 * @brief 数据管理器类
 * 
 * 负责 SQLite 数据库的初始化、数据写入和管理。
 * 
 * 主要功能：
 * 1. 初始化数据库表结构 (DetectionTask, MotionLog)。
 * 2. 记录实时运动数据 (位置、速度、状态)。
 * 3. 创建和管理检测任务记录。
 * 
 * 线程安全说明：
 * SQLite 的连接通常是线程绑定的。本类提供了 getConnectionName() 方法
 * 确保每个线程使用独立的数据库连接实例，以支持多线程并发访问（如果需要）。
 */
class DataManager : public QObject
{
    Q_OBJECT
public:
    explicit DataManager(QObject *parent = nullptr);
    ~DataManager();

    /**
     * @brief 初始化数据库
     * 
     * 如果数据库文件不存在会自动创建。
     * 同时会检查并创建所需的表结构。
     * @return true 初始化成功, false 失败
     */
    bool initDatabase();

    /**
     * @brief 获取当前线程的数据库连接名称
     */
    QString connectionName() const;

public slots:
    /**
     * @brief 记录运动日志
     * @param fb 运动反馈数据
     * @param taskId 关联的任务ID (默认为-1，表示无任务)
     */
    void logMotionData(const MotionFeedback &fb, int taskId = -1);
    
    /**
     * @brief 创建新的检测任务
     * @param operatorName 操作员姓名
     * @param tubeId 管道编号
     * @return 新创建的任务ID，失败返回 -1
     */
    int createDetectionTask(const QString &operatorName, const QString &tubeId);

private:
    /**
     * @brief 获取当前线程的数据库连接名称 (Internal)
     * 使用线程ID作为后缀，确保连接不跨线程共享。
     */
    QString getConnectionName() const;

    QString m_dbPath; ///< 数据库文件路径
};

#endif // DATAMANAGER_H