#include "devicecontroller.h"
#include "../utils/logger.h"

DeviceController::DeviceController(QObject *parent) : QObject(parent)
{
    m_serialManager = new SerialManager(); // 不能指定父对象，因为要移动到新线程
    m_dataManager = new DataManager(this); // 数据管理在主线程（简单起见，仅写入由主线程触发）
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
}

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
    // 1. 记录数据到数据库
    m_dataManager->logMotionData(fb);

    // 2. 更新 UI
    emit deviceStateUpdated(fb);
}