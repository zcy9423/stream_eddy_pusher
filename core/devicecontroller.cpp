#include "devicecontroller.h"
#include "../utils/logger.h"
#include "configmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

DeviceController::DeviceController(QObject *parent) : QObject(parent)
{
    m_commManager = new CommunicationManager(); // 不能指定父对象，因为要移动到新线程
    m_dataManager = new DataManager(this); // 数据管理在主线程
    m_taskManager = new TaskManager(this);
}

DeviceController::~DeviceController()
{
    if (m_commManager) {
        if (m_workerThread.isRunning()) {
            QMetaObject::invokeMethod(m_commManager, "closeConnection", Qt::BlockingQueuedConnection);
            QMetaObject::invokeMethod(m_commManager, "deleteLater", Qt::BlockingQueuedConnection);
            m_commManager = nullptr;
        } else {
            m_commManager->closeConnection();
            delete m_commManager;
            m_commManager = nullptr;
        }
    }
    m_workerThread.quit();
    m_workerThread.wait();
}

void DeviceController::init()
{
    if (!m_dataManager->initDatabase()) {
        emit errorMessage("数据库初始化失败！");
    }

    m_commManager->moveToThread(&m_workerThread);

    // 1. Controller -> CommManager
    connect(this, &DeviceController::cmdOpenConnection, m_commManager, &CommunicationManager::openConnection);
    connect(this, &DeviceController::cmdCloseConnection, m_commManager, &CommunicationManager::closeConnection);
    connect(this, &DeviceController::cmdSendPacket, m_commManager, &CommunicationManager::processCommand);

    // 2. CommManager -> Controller
    connect(m_commManager, &CommunicationManager::connectionOpened, this, [this](bool success){
        emit connectionChanged(success);
    });
    connect(m_commManager, &CommunicationManager::connectionError, this, &DeviceController::errorMessage);
    connect(m_commManager, &CommunicationManager::feedbackReceived, this, &DeviceController::onFeedbackReceived);

    m_workerThread.start();

    // 3. Controller 内部连接
    connect(this, &DeviceController::deviceStateUpdated, this, [this](MotionFeedback fb){
        m_taskManager->onPositionUpdated(fb.position_mm);
    });

    // 4. TaskManager -> Controller
    connect(m_taskManager, &TaskManager::requestMoveForward, this, [this](double speed){
        ControlCommand cmd;
        cmd.type = ControlCommand::MoveForward;
        cmd.param = speed;
        emit cmdSendPacket(cmd);
    });

    connect(m_taskManager, &TaskManager::requestMoveBackward, this, [this](double speed){
        ControlCommand cmd;
        cmd.type = ControlCommand::MoveBackward;
        cmd.param = speed;
        emit cmdSendPacket(cmd);
    });

    connect(m_taskManager, &TaskManager::requestStop, this, [this](){
        ControlCommand cmd;
        cmd.type = ControlCommand::Stop;
        emit cmdSendPacket(cmd);
    });
    
    // 处理任务完成
    connect(m_taskManager, &TaskManager::taskCompleted, this, [this](){
        if (m_currentTaskId != -1) {
            // 生成执行结果JSON
            QJsonObject result;
            result["completionTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            result["status"] = "success";
            result["message"] = "任务执行完成";
            
            QJsonDocument doc(result);
            QString resultJson = doc.toJson(QJsonDocument::Compact);
            
            // 更新数据库中的任务状态和执行结果
            updateTaskStatus(m_currentTaskId, "completed");
            m_dataManager->updateTaskExecutionResult(m_currentTaskId, resultJson);
            
            LOG_INFO << "任务完成: ID=" << m_currentTaskId;
            
            // 先发送任务状态变更信号（包含具体的taskId）
            int completedTaskId = m_currentTaskId;
            // 清除当前任务ID
            m_currentTaskId = -1;
            emit taskStateChanged(completedTaskId);
        }
    });
    
    // 处理任务失败
    connect(m_taskManager, &TaskManager::taskFailed, this, [this](const QString& reason){
        if (m_currentTaskId != -1) {
            // 生成失败结果JSON
            QJsonObject result;
            result["completionTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
            result["status"] = "failed";
            result["message"] = reason;
            
            QJsonDocument doc(result);
            QString resultJson = doc.toJson(QJsonDocument::Compact);
            
            // 更新数据库中的任务状态和执行结果
            updateTaskStatus(m_currentTaskId, "failed");
            m_dataManager->updateTaskExecutionResult(m_currentTaskId, resultJson);
            
            LOG_ERR << "任务失败: ID=" << m_currentTaskId << " 原因:" << reason;
            
            // 先发送任务状态变更信号（包含具体的taskId）
            int failedTaskId = m_currentTaskId;
            // 清除当前任务ID
            m_currentTaskId = -1;
            emit taskStateChanged(failedTaskId);
        }
    });
}

void DeviceController::startAutoScan(double min, double max, double speed, int cycles) {
    if(!m_taskManager->isRunning()) {
        m_taskManager->startAutoScan(min, max, speed, cycles);
    }
}

void DeviceController::pauseAutoScan() { m_taskManager->pause(); }
void DeviceController::resumeAutoScan() { m_taskManager->resume(); }
void DeviceController::resetAutoScan() { m_taskManager->resetTask(); }
void DeviceController::stopAutoScan() { m_taskManager->stopAll(); }

void DeviceController::requestConnect(int type, const QString &addr, int portOrBaud)
{
    emit cmdOpenConnection(type, addr, portOrBaud);
}

void DeviceController::requestDisconnect()
{
    emit cmdCloseConnection();
}

void DeviceController::manualMove(bool forward, double speed)
{
    ControlCommand cmd;
    cmd.type = forward ? ControlCommand::MoveForward : ControlCommand::MoveBackward;
    cmd.param = speed;
    emit cmdSendPacket(cmd);
}

void DeviceController::stopMotion()
{
    ControlCommand cmd;
    cmd.type = ControlCommand::Stop;
    emit cmdSendPacket(cmd);
}

void DeviceController::setSpeed(double speed)
{
    ControlCommand cmd;
    cmd.type = ControlCommand::SetSpeed;
    cmd.param = speed;
    emit cmdSendPacket(cmd);
}

void DeviceController::onFeedbackReceived(MotionFeedback fb)
{
    // 检查软限位保护
    double maxPos = ConfigManager::instance().maxPosition();
    
    if (m_taskManager->isRunning()) {
        // 自动任务运行时：只有明显超出限位才停止（留一点容差）
        double tolerance = 0.5; // 0.5mm容差
        
        // 左限位保护：超出左限位容差范围
        if (fb.status == DeviceStatus::MovingBackward && fb.position_mm < (0.0 - tolerance)) {
            stopMotion();
            m_taskManager->stopAll();
            emit errorMessage(QString("⚠️ 超出左限位保护范围 (%1mm)，自动停止！").arg(0.0 - tolerance));
        }
        
        // 右限位保护：超出右限位容差范围
        if (fb.status == DeviceStatus::MovingForward && fb.position_mm > (maxPos + tolerance)) {
            stopMotion();
            m_taskManager->stopAll();
            emit errorMessage(QString("⚠️ 超出右限位保护范围 (%1mm)，自动停止！").arg(maxPos + tolerance));
        }
    } else {
        // 手动控制时：严格限位保护
        // 左限位保护 (拉回时 <= 0)
        if (fb.status == DeviceStatus::MovingBackward && fb.position_mm <= 0.0) {
            stopMotion();
            emit errorMessage("⚠️ 已到达左限位 (0mm)，自动停止！");
        }
        
        // 右限位保护 (推进时 >= Max)
        if (fb.status == DeviceStatus::MovingForward && fb.position_mm >= maxPos) {
            stopMotion();
            emit errorMessage(QString("⚠️ 已到达右限位 (%1mm)，自动停止！").arg(maxPos));
        }
    }

    QMetaObject::invokeMethod(m_dataManager, "logMotionData", 
                              Qt::QueuedConnection, 
                              Q_ARG(MotionFeedback, fb),
                              Q_ARG(int, m_currentTaskId));

    m_taskManager->updateFeedback(fb);
    emit deviceStateUpdated(fb);
    
    if (fb.emergencyStop || fb.overCurrent || fb.stalled) {
        m_taskManager->stopAll();
        stopMotion();
        
        QString reason;
        if (fb.emergencyStop) reason += "[急停按钮按下] ";
        if (fb.overCurrent) reason += "[电机过流] ";
        if (fb.stalled) reason += "[电机堵转] ";
        
        static qint64 lastAlarmTime = 0;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastAlarmTime > 2000) {
            emit errorMessage("CRITICAL ALARM: " + reason);
            lastAlarmTime = now;
        }
    }
}

void DeviceController::activateTask(int taskId)
{
    if (taskId == -1 || taskId == m_currentTaskId) {
        return;
    }
    m_currentTaskId = taskId;
    emit taskStateChanged(m_currentTaskId);
}

void DeviceController::startNewTask(const QString &operatorName, const QString &tubeId)
{
    int newId = m_dataManager->createDetectionTask(operatorName, tubeId);
    if (newId != -1) {
        m_currentTaskId = newId;
        LOG_INFO << "任务开始: ID=" << newId << " 操作员=" << operatorName << " 管号=" << tubeId;
        // 先发送创建信号，方便 UI 更新列表
        emit taskCreated(newId, operatorName, tubeId);
        // 再发送状态变更信号
        emit taskStateChanged(m_currentTaskId);
    } else {
        LOG_ERR << "创建任务失败";
        emit errorMessage("创建任务记录失败，数据将不会关联到具体管道！");
    }
}

void DeviceController::endCurrentTask()
{
    if (m_currentTaskId != -1) {
        LOG_INFO << "任务结束: ID=" << m_currentTaskId;
        if (m_dataManager) {
            m_dataManager->updateDetectionTaskStatus(m_currentTaskId, "stop");
        }
        m_currentTaskId = -1;
        emit taskStateChanged(m_currentTaskId);
    }
}

bool DeviceController::updateTaskStatus(int taskId, const QString &status)
{
    if (!m_dataManager) return false;
    return m_dataManager->updateDetectionTaskStatus(taskId, status);
}

bool DeviceController::deleteTask(int taskId)
{
    if (!m_dataManager) return false;

    if (taskId == m_currentTaskId) {
        m_taskManager->stopAll();
        stopMotion();
        m_currentTaskId = -1;
        emit taskStateChanged(m_currentTaskId);
    }

    const bool ok = m_dataManager->deleteDetectionTask(taskId);
    if (ok) {
        LOG_INFO << "任务删除: ID=" << taskId;
    }
    return ok;
}
