#include "datamanager.h"
#include "../utils/logger.h"
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QThread>

DataManager::DataManager(QObject *parent) : QObject(parent)
{
}

DataManager::~DataManager()
{
    // 在析构时移除当前线程的数据库连接
    QString connName = getConnectionName();
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase::removeDatabase(connName);
    }
}

/**
 * @brief 获取线程特定的数据库连接名称
 * 
 * 这里的策略是使用 "Connection_" 加上当前线程的 ID。
 * 这样可以确保每个线程都有自己独立的连接名，避免 Qt 报 "connection in use" 警告。
 */
QString DataManager::getConnectionName() const
{
    // 使用内存地址作为唯一标识符，防止多线程冲突
    return QString("Connection_%1").arg((quint64)QThread::currentThreadId());
}

/**
 * @brief 初始化数据库
 * 
 * 1. 创建或获取数据库连接。
 * 2. 启用 SQLite 驱动。
 * 3. 创建必要的表：DetectionTask 和 MotionLog。
 */
bool DataManager::initDatabase()
{
    QString connName = getConnectionName();
    QSqlDatabase db;
    
    if (QSqlDatabase::contains(connName)) {
        db = QSqlDatabase::database(connName);
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(m_dbPath);
    }

    if (!db.open()) {
        LOG_ERR << "数据库打开失败：" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    
    // 表1: 任务表 - 存储每次检测任务的元数据
    bool success = query.exec("CREATE TABLE IF NOT EXISTS DetectionTask ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "start_time DATETIME, "
                              "operator_name TEXT, "
                              "tube_id TEXT)");
    if (!success) LOG_ERR << "创建任务表失败：" << query.lastError().text();

    // 表2: 运动日志表 - 存储高频的运动状态数据
    // 注意：在实际高频写入场景中，可能需要考虑分表或定期归档
    success = query.exec("CREATE TABLE IF NOT EXISTS MotionLog ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "task_id INTEGER, "
                         "timestamp DATETIME, "
                         "position REAL, "
                         "speed REAL, "
                         "status INTEGER)");
                         
    if (!success) LOG_ERR << "创建日志失败：" << query.lastError().text();

    return success;
}

/**
 * @brief 记录单条运动数据
 * 
 * 将当前的时间戳、位置、速度和状态写入 MotionLog 表。
 */
void DataManager::logMotionData(const MotionFeedback &fb)
{
    // 注意：实际生产中如果数据频率很高（例如 > 100Hz），
    // 建议使用事务 (Transaction) 进行批量插入，或者先写入内存队列，由单独线程批量刷入数据库。
    // 这里演示简单的单条插入模式。
    
    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("INSERT INTO MotionLog (timestamp, position, speed, status) "
                  "VALUES (:time, :pos, :spd, :stat)");
    query.bindValue(":time", QDateTime::currentDateTime());
    query.bindValue(":pos", fb.position_mm);
    query.bindValue(":spd", fb.speed_mm_s);
    query.bindValue(":stat", static_cast<int>(fb.status));
    
    if (!query.exec()) {
        // 记录错误，但避免日志泛滥
        // LOG_WARN << "Insert Log Error:" << query.lastError().text();
    }
}

/**
 * @brief 创建新的检测任务
 * 
 * 在开始新的检测流程前调用，返回生成的任务 ID，以便后续关联日志数据。
 */
int DataManager::createDetectionTask(const QString &operatorName, const QString &tubeId)
{
    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    QSqlQuery query(db);
    
    query.prepare("INSERT INTO DetectionTask (start_time, operator_name, tube_id) "
                  "VALUES (:time, :op, :tube)");
    query.bindValue(":time", QDateTime::currentDateTime());
    query.bindValue(":op", operatorName);
    query.bindValue(":tube", tubeId);
    
    if (query.exec()) {
        return query.lastInsertId().toInt();
    }
    return -1;
}