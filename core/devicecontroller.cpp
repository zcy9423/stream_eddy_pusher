#include "devicecontroller.h"
#include "../utils/logger.h"

DeviceController::DeviceController(QObject *parent) : QObject(parent)
{
    m_serialManager = new SerialManager(); // 不能指定父对象，因为要移动到新线程
    m_dataManager = new DataManager(this); // 数据管理在主线程（简单起见，仅写入由主线程触发）
    // 提前初始化任务管理器，确保 MainWindow::initLogic() 能获取到有效指针
    m_taskManager = new TaskManager(this);
}

DeviceController::~DeviceController()
{
    // 优雅退出线程
    m_workerThread.quit();
    m_workerThread.wait();
    
    // SerialManager 被移动到了 m_workerThread，线程结束后需要手动删除（或通过 deleteLater）
    delete m_serialManager; 
}

/**
 * @brief 初始化系统
 * 
 * 1. 初始化数据库。
 * 2. 配置并启动后台通信线程。
 * 3. 建立跨线程的信号槽连接。
 */
void DeviceController::init()
{
    // 初始化数据库
    if (!m_dataManager->initDatabase()) {
        emit errorMessage("数据库初始化失败！");
    }

    // 将串口管理对象移动到工作线程
    // 这样 SerialManager 的槽函数就会在该线程中执行，避免阻塞主线程 UI
    m_serialManager->moveToThread(&m_workerThread);

    // 连接信号槽 (跨线程)
    // 1. Controller (主线程) -> SerialManager (工作线程)
    connect(this, &DeviceController::cmdOpenPort, m_serialManager, &SerialManager::openPort);
    connect(this, &DeviceController::cmdClosePort, m_serialManager, &SerialManager::closePort);
    connect(this, &DeviceController::cmdSendPacket, m_serialManager, &SerialManager::processCommand);

    // 2. SerialManager (工作线程) -> Controller (主线程)
    connect(m_serialManager, &SerialManager::portOpened, this, [this](bool success){
        emit connectionChanged(success);
    });
    connect(m_serialManager, &SerialManager::connectionError, this, &DeviceController::errorMessage);
    connect(m_serialManager, &SerialManager::feedbackReceived, this, &DeviceController::onFeedbackReceived);

    // 启动线程
    m_workerThread.start();

    // 1. 将硬件位置反馈喂给任务管理器
    // 注意：deviceStateUpdated 是本类发出的信号，当收到 SerialManager 反馈时触发
    connect(this, &DeviceController::deviceStateUpdated, this, [this](MotionFeedback fb){
        // 更新任务管理器的位置感知
        // 注意：如果 fb.status 是 ERROR，也许应该通知 TaskManager 停止？
        m_taskManager->onPositionUpdated(fb.position_mm);
    });

    // 2. 将任务管理器的控制请求 -> 转译为硬件指令
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
}

// 转发 UI 的调用
void DeviceController::startAutoScan(double min, double max, double speed, int cycles) {
    if(!m_taskManager->isRunning()) {
        m_taskManager->startAutoScan(min, max, speed, cycles);
    }
}

void DeviceController::pauseAutoScan() { m_taskManager->pause(); }
void DeviceController::resumeAutoScan() { m_taskManager->resume(); }
void DeviceController::stopAutoScan() { m_taskManager->stopAll(); }

/**
 * @brief 请求连接设备
 * 发送信号通知后台线程打开串口。
 */
void DeviceController::requestConnect(const QString &port, int baud)
{
    emit cmdOpenPort(port, baud);
}

/**
 * @brief 请求断开设备连接
 */
void DeviceController::requestDisconnect()
{
    emit cmdClosePort();
}

/**
 * @brief 手动控制设备移动
 */
void DeviceController::manualMove(bool forward, double speed)
{
    ControlCommand cmd;
    cmd.type = forward ? ControlCommand::MoveForward : ControlCommand::MoveBackward;
    cmd.param = speed;
    emit cmdSendPacket(cmd);
}

/**
 * @brief 停止设备运动
 */
void DeviceController::stopMotion()
{
    ControlCommand cmd;
    cmd.type = ControlCommand::Stop;
    emit cmdSendPacket(cmd);
}

/**
 * @brief 设置目标速度
 */
void DeviceController::setSpeed(double speed)
{
    ControlCommand cmd;
    cmd.type = ControlCommand::SetSpeed;
    cmd.param = speed;
    emit cmdSendPacket(cmd);
}

/**
 * @brief 处理设备反馈数据
 * 
 * 1. 将数据写入数据库日志。
 * 2. 将数据转发给 UI 层显示。
 */
void DeviceController::onFeedbackReceived(MotionFeedback fb)
{
    // 1. 记录到数据库
    // 异步调用，以免阻塞串口处理线程
    QMetaObject::invokeMethod(m_dataManager, "logMotionData", 
                              Qt::QueuedConnection, 
                              Q_ARG(MotionFeedback, fb),
                              Q_ARG(int, m_currentTaskId));

    // 2. 通知任务管理器更新状态 (用于闭环控制)
    m_taskManager->updateFeedback(fb);

    // 3. 转发给 UI 显示
    emit deviceStateUpdated(fb);
    
    // 4. 急停与报警逻辑检测
    if (fb.emergencyStop || fb.overCurrent || fb.stalled) {
        // 硬件已触发急停，软件层也必须立即响应
        // 1. 停止所有软件层面的自动任务
        m_taskManager->stopAll();
        
        // 2. 向下位机发送软件停止指令（双重保险）
        // 虽然通常硬件急停会切断电机，但发送Stop指令可以清除下位机的运行状态
        stopMotion();
        
        QString reason;
        if (fb.emergencyStop) reason += "[急停按钮按下] ";
        if (fb.overCurrent) reason += "[电机过流] ";
        if (fb.stalled) reason += "[电机堵转] ";
        
        // 避免重复频繁弹窗，这里可以加一个状态判断或限流
        // 简单处理：通过 errorMessage 发送，UI层决定是否弹窗
        // 为了强调严重性，我们发送一个带前缀的错误
        static qint64 lastAlarmTime = 0;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastAlarmTime > 2000) { // 2秒内不重复报同类错误
            emit errorMessage("CRITICAL ALARM: " + reason);
            lastAlarmTime = now;
        }
    }
}

void DeviceController::startNewTask(const QString &operatorName, const QString &tubeId)
{
    // DataManager 在主线程，直接调用即可
    int newId = m_dataManager->createDetectionTask(operatorName, tubeId);
    
    if (newId != -1) {
        m_currentTaskId = newId;
        LOG_INFO << "任务开始: ID=" << newId << " 操作员=" << operatorName << " 管号=" << tubeId;
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
        m_currentTaskId = -1;
        emit taskStateChanged(m_currentTaskId);
    }
}