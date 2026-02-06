#include "datamanager.h"
#include "../utils/logger.h"
#include "../core/configmanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QThread>

DataManager::DataManager(QObject *parent) : QObject(parent)
{
    m_dbPath = "EddyPusher.db"; // 默认路径
}

DataManager::~DataManager()
{
    // 在析构时移除当前线程的数据库连接
    QString connName = getConnectionName();
    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase::removeDatabase(connName);
    }
}

QString DataManager::connectionName() const
{
    return getConnectionName();
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
    LOG_INFO << "========== 初始化数据库 ==========";
    QString connName = getConnectionName();
    LOG_INFO << "数据库连接名: " << connName;
    QSqlDatabase db;
    
    // 从配置中获取存储路径
    QString dataDir = ConfigManager::instance().dataStoragePath();
    m_dbPath = dataDir + "/EddyPusher.db";
    LOG_INFO << "数据库路径: " << m_dbPath;

    if (QSqlDatabase::contains(connName)) {
        LOG_INFO << "使用已存在的数据库连接";
        db = QSqlDatabase::database(connName);
    } else {
        LOG_INFO << "创建新的数据库连接";
        db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(m_dbPath);
    }

    if (!db.open()) {
        LOG_ERR << "数据库打开失败：" << db.lastError().text();
        return false;
    }
    LOG_INFO << "数据库打开成功";

    QSqlQuery query(db);
    
    // 表1: 任务表 - 存储每次检测任务的元数据
    LOG_INFO << "创建/检查 DetectionTask 表";
    bool success = query.exec("CREATE TABLE IF NOT EXISTS DetectionTask ("
                              "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                              "start_time DATETIME, "
                              "operator_name TEXT, "
                              "tube_id TEXT, "
                              "status TEXT DEFAULT 'create', "
                              "task_type TEXT DEFAULT 'manual', "  // 任务类型：manual, auto_scan, sequence
                              "task_config TEXT, "                 // 任务配置JSON
                              "execution_result TEXT, "            // 执行结果JSON
                              "completion_time DATETIME)");        // 完成时间
    if (!success) {
        LOG_ERR << "创建任务表失败：" << query.lastError().text();
    } else {
        LOG_INFO << "DetectionTask 表就绪";
    }

    bool hasStatus = false;
    bool hasTaskType = false;
    bool hasTaskConfig = false;
    bool hasExecutionResult = false;
    bool hasCompletionTime = false;
    
    if (query.exec("PRAGMA table_info(DetectionTask)")) {
        while (query.next()) {
            QString fieldName = query.value("name").toString();
            if (fieldName == "status") hasStatus = true;
            else if (fieldName == "task_type") hasTaskType = true;
            else if (fieldName == "task_config") hasTaskConfig = true;
            else if (fieldName == "execution_result") hasExecutionResult = true;
            else if (fieldName == "completion_time") hasCompletionTime = true;
        }
    }

    // 添加缺失的字段
    if (!hasStatus) {
        QSqlQuery alterQuery(db);
        if (!alterQuery.exec("ALTER TABLE DetectionTask ADD COLUMN status TEXT DEFAULT 'create'")) {
            LOG_ERR << "新增状态列失败：" << alterQuery.lastError().text();
        }
    }
    if (!hasTaskType) {
        QSqlQuery alterQuery(db);
        if (!alterQuery.exec("ALTER TABLE DetectionTask ADD COLUMN task_type TEXT DEFAULT 'manual'")) {
            LOG_ERR << "新增任务类型列失败：" << alterQuery.lastError().text();
        }
    }
    if (!hasTaskConfig) {
        QSqlQuery alterQuery(db);
        if (!alterQuery.exec("ALTER TABLE DetectionTask ADD COLUMN task_config TEXT")) {
            LOG_ERR << "新增任务配置列失败：" << alterQuery.lastError().text();
        }
    }
    if (!hasExecutionResult) {
        QSqlQuery alterQuery(db);
        if (!alterQuery.exec("ALTER TABLE DetectionTask ADD COLUMN execution_result TEXT")) {
            LOG_ERR << "新增执行结果列失败：" << alterQuery.lastError().text();
        }
    }
    if (!hasCompletionTime) {
        QSqlQuery alterQuery(db);
        if (!alterQuery.exec("ALTER TABLE DetectionTask ADD COLUMN completion_time DATETIME")) {
            LOG_ERR << "新增完成时间列失败：" << alterQuery.lastError().text();
        }
    }

    if (!query.exec("UPDATE DetectionTask SET status = 'stop' "
                    "WHERE (status IS NULL OR status = '') "
                    "AND id IN (SELECT DISTINCT task_id FROM MotionLog WHERE task_id IS NOT NULL)")) {
        LOG_ERR << "修正任务状态失败：" << query.lastError().text();
    }
    if (!query.exec("UPDATE DetectionTask SET status = 'create' "
                    "WHERE (status IS NULL OR status = '') "
                    "AND id NOT IN (SELECT DISTINCT task_id FROM MotionLog WHERE task_id IS NOT NULL)")) {
        LOG_ERR << "修正任务状态失败：" << query.lastError().text();
    }

    // 表2: 运动日志表 - 存储高频的运动状态数据
    // 注意：在实际高频写入场景中，可能需要考虑分表或定期归档
    LOG_INFO << "创建/检查 MotionLog 表";
    success = query.exec("CREATE TABLE IF NOT EXISTS MotionLog ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "task_id INTEGER, "
                         "timestamp DATETIME, "
                         "position REAL, "
                         "speed REAL, "
                         "status INTEGER)");
                         
    if (!success) {
        LOG_ERR << "创建日志失败：" << query.lastError().text();
    } else {
        LOG_INFO << "MotionLog 表就绪";
    }

    // 初始化完成后，执行一次数据清理 (自动维护策略)
    LOG_INFO << "执行数据清理 (保留最近30天数据)";
    cleanupOldData(30);

    LOG_INFO << "数据库初始化完成";
    return success;
}

/**
 * @brief 自动清理旧数据
 * 删除 MotionLog 和 DetectionTask 表中超过指定天数的数据，
 * 防止数据库文件无限增长。
 */
void DataManager::cleanupOldData(int daysToKeep)
{
    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    
    // 计算截止时间点
    QDateTime cutoffTime = QDateTime::currentDateTime().addDays(-daysToKeep);
    
    // 1. 清理运动日志 (MotionLog)
    query.prepare("DELETE FROM MotionLog WHERE timestamp < :cutoff");
    query.bindValue(":cutoff", cutoffTime);
    if (query.exec()) {
        int deleted = query.numRowsAffected();
        if (deleted > 0) {
            LOG_INFO << "自动清理: 已删除" << deleted << "条过期的运动日志 (超过" << daysToKeep << "天)";
        }
    } else {
        LOG_ERR << "清理 MotionLog 失败:" << query.lastError().text();
    }

    // 2. 清理任务记录 (DetectionTask)
    query.prepare("DELETE FROM DetectionTask WHERE start_time < :cutoff");
    query.bindValue(":cutoff", cutoffTime);
    if (query.exec()) {
        int deleted = query.numRowsAffected();
        if (deleted > 0) {
            LOG_INFO << "自动清理: 已删除" << deleted << "条过期的任务记录";
        }
    } else {
        LOG_ERR << "清理 DetectionTask 失败:" << query.lastError().text();
    }
    
    // 3. 执行 VACUUM 释放磁盘空间 (可选，操作较重，建议在空闲时执行)
    // query.exec("VACUUM");
}

/**
 * @brief 记录单条运动数据
 * 
 * 将当前的时间戳、位置、速度和状态写入 MotionLog 表。
 */
void DataManager::logMotionData(const MotionFeedback &fb, int taskId)
{
    // 注意：实际生产中如果数据频率很高（例如 > 100Hz），
    // 建议使用事务 (Transaction) 进行批量插入，或者先写入内存队列，由单独线程批量刷入数据库。
    // 这里演示简单的单条插入模式。
    
    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("INSERT INTO MotionLog (timestamp, position, speed, status, task_id) "
                  "VALUES (:time, :pos, :spd, :stat, :tid)");
    query.bindValue(":time", QDateTime::currentDateTime());
    query.bindValue(":pos", fb.position_mm);
    query.bindValue(":spd", fb.speed_mm_s);
    query.bindValue(":stat", static_cast<int>(fb.status));
    query.bindValue(":tid", taskId == -1 ? QVariant() : taskId);
    
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
    LOG_INFO << "========== 创建检测任务 ==========";
    LOG_INFO << "操作员: " << operatorName << ", 管号: " << tubeId;
    
    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    QSqlQuery query(db);
    
    query.prepare("INSERT INTO DetectionTask (start_time, operator_name, tube_id, status) "
                  "VALUES (:time, :op, :tube, :status)");
    query.bindValue(":time", QDateTime::currentDateTime());
    query.bindValue(":op", operatorName);
    query.bindValue(":tube", tubeId);
    query.bindValue(":status", "create");
    
    if (query.exec()) {
        int taskId = query.lastInsertId().toInt();
        LOG_INFO << "任务创建成功，ID: " << taskId;
        return taskId;
    }
    LOG_ERR << "任务创建失败: " << query.lastError().text();
    return -1;
}

bool DataManager::updateDetectionTaskStatus(int taskId, const QString &status)
{
    if (taskId <= 0) return false;

    LOG_INFO << "更新任务状态 - ID: " << taskId << ", 新状态: " << status;

    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE DetectionTask SET status = :status WHERE id = :tid");
    query.bindValue(":status", status);
    query.bindValue(":tid", taskId);
    if (!query.exec()) {
        LOG_ERR << "更新任务状态失败:" << query.lastError().text();
        return false;
    }
    LOG_INFO << "任务状态更新成功";
    return true;
}

bool DataManager::deleteDetectionTask(int taskId)
{
    if (taskId <= 0) return false;

    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("DELETE FROM MotionLog WHERE task_id = :tid");
    query.bindValue(":tid", taskId);
    if (!query.exec()) {
        LOG_ERR << "删除 MotionLog 失败:" << query.lastError().text();
        return false;
    }

    query.prepare("DELETE FROM DetectionTask WHERE id = :tid");
    query.bindValue(":tid", taskId);
    if (!query.exec()) {
        LOG_ERR << "删除 DetectionTask 失败:" << query.lastError().text();
        return false;
    }

    return true;
}
bool DataManager::updateTaskConfig(int taskId, const QString &taskType, const QString &taskConfig)
{
    if (taskId <= 0) return false;

    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE DetectionTask SET task_type = :type, task_config = :config WHERE id = :tid");
    query.bindValue(":type", taskType);
    query.bindValue(":config", taskConfig);
    query.bindValue(":tid", taskId);
    
    if (!query.exec()) {
        LOG_ERR << "更新任务配置失败:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DataManager::getTaskConfig(int taskId, QString &taskType, QString &taskConfig)
{
    if (taskId <= 0) return false;

    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("SELECT task_type, task_config FROM DetectionTask WHERE id = :tid");
    query.bindValue(":tid", taskId);
    
    if (query.exec() && query.next()) {
        taskType = query.value("task_type").toString();
        taskConfig = query.value("task_config").toString();
        return true;
    }
    return false;
}

bool DataManager::updateTaskExecutionResult(int taskId, const QString &executionResult)
{
    if (taskId <= 0) return false;

    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("UPDATE DetectionTask SET execution_result = :result, completion_time = :time WHERE id = :tid");
    query.bindValue(":result", executionResult);
    query.bindValue(":time", QDateTime::currentDateTime());
    query.bindValue(":tid", taskId);
    
    if (!query.exec()) {
        LOG_ERR << "更新任务执行结果失败:" << query.lastError().text();
        return false;
    }
    return true;
}

QString DataManager::getTaskExecutionResult(int taskId)
{
    if (taskId <= 0) return QString();

    QString connName = getConnectionName();
    QSqlDatabase db = QSqlDatabase::database(connName);
    if (!db.isOpen()) return QString();

    QSqlQuery query(db);
    query.prepare("SELECT execution_result FROM DetectionTask WHERE id = :tid");
    query.bindValue(":tid", taskId);
    
    if (query.exec() && query.next()) {
        return query.value("execution_result").toString();
    }
    return QString();
}