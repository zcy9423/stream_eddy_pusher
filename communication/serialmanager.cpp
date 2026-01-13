#include "serialmanager.h"
#include "../utils/logger.h"

SerialManager::SerialManager(QObject *parent) : QObject(parent)
{
    // 构造函数中不初始化 QSerialPort，因为对象可能被移动到新线程
    // 资源初始化应在 openPort 或第一次使用时进行，以确保对象归属正确的线程
}

SerialManager::~SerialManager()
{
    closePort();
}

/**
 * @brief 打开串口连接
 * 
 * 根据端口名称判断使用仿真模式还是真实模式：
 * - "COM_VIRTUAL": 启动仿真模式
 * - 其他: 启动真实串口连接
 */
void SerialManager::openPort(const QString &portName, int baudRate)
{
    // 先关闭当前可能存在的连接
    closePort();

    // 判断模式
    if (portName == "COM_VIRTUAL") {
        m_isSimulating = true;
        LOG_INFO << "启动仿真模式，虚拟端口:" << portName;

        if (!m_simTimer) {
            m_simTimer = new QTimer(this);
            connect(m_simTimer, &QTimer::timeout, this, &SerialManager::handleSimTimeout);
        }
        
        // 初始化仿真状态
        m_simState.status = DeviceStatus::Idle;
        m_simState.position_mm = 0.0;
        m_simTimer->start(100); // 10Hz 刷新率 (每100ms一次)
        
        emit portOpened(true);
    } 
    else {
        m_isSimulating = false;
        
        // 真实串口模式
        if (!m_serial) {
            m_serial = new QSerialPort(this);
            connect(m_serial, &QSerialPort::readyRead, this, &SerialManager::handleReadyRead);
        }

        m_serial->setPortName(portName);
        m_serial->setBaudRate(baudRate);
        
        if (m_serial->open(QIODevice::ReadWrite)) {
            LOG_INFO << "串口已打开：" << portName;
            emit portOpened(true);
        } else {
            LOG_ERR << "打开串口失败：" << m_serial->errorString();
            emit portOpened(false);
            emit connectionError(m_serial->errorString());
        }
    }
}

/**
 * @brief 关闭串口连接
 * 停止仿真定时器或关闭真实串口。
 */
void SerialManager::closePort()
{
    if (m_isSimulating) {
        if (m_simTimer) {
            m_simTimer->stop();
        }
        LOG_INFO << "仿真已停止。";
    } 
    else {
        if (m_serial && m_serial->isOpen()) {
            m_serial->close();
            LOG_INFO << "串口已关闭。";
        }
    }
    
    // 重置模式标志
    m_isSimulating = false;

    // 关闭端口后必须通知 UI 连接已断开
    emit portOpened(false);
}

/**
 * @brief 处理控制指令
 * 
 * 接收来自上层的指令，并将其转换为设备协议数据发送（或更新仿真状态）。
 */
void SerialManager::processCommand(ControlCommand cmd)
{
    if (m_isSimulating) {
        // --- 仿真模式逻辑 ---
        switch (cmd.type) {
        case ControlCommand::MoveForward:
            m_simState.status = DeviceStatus::MovingForward;
            m_simTargetSpeed = cmd.param > 0 ? cmd.param : 10.0;
            break;
        case ControlCommand::MoveBackward:
            m_simState.status = DeviceStatus::MovingBackward;
            m_simTargetSpeed = cmd.param > 0 ? cmd.param : 10.0;
            break;
        case ControlCommand::Stop:
            m_simState.status = DeviceStatus::Idle;
            m_simTargetSpeed = 0.0;
            break;
        case ControlCommand::SetSpeed:
            m_simTargetSpeed = cmd.param;
            break;
        }
        LOG_INFO << "[Sim] 接收命令类型：" << cmd.type << " 参数：" << cmd.param;
    } 
    else {
        // --- 真实模式逻辑 ---
        if (!m_serial || !m_serial->isOpen()) return;
        
        // TODO: 实现具体的通信协议封装
        // QByteArray packet = Protocol::pack(cmd); 
        // m_serial->write(packet);
        
        LOG_INFO << "[Real] 发送命令类型：" << cmd.type << " 参数：" << cmd.param;
    }
}

/**
 * @brief 处理真实串口数据接收
 */
void SerialManager::handleReadyRead()
{
    // 真实数据解析逻辑
    if (!m_serial) return;
    
    QByteArray data = m_serial->readAll();
    // MotionFeedback fb = Protocol::parse(data);
    // emit feedbackReceived(fb);
    
    // 调试打印
    // LOG_INFO << "收到真实串口数据，长度：" << data.size();
}

/**
 * @brief 仿真器定时回调
 * 
 * 简单模拟物理运动模型：根据速度更新位置。
 */
void SerialManager::handleSimTimeout()
{
    if (!m_isSimulating) return;

    // 简单的物理运动模拟
    if (m_simState.status == DeviceStatus::MovingForward) {
        m_simState.position_mm += (m_simTargetSpeed * 0.1); // speed * dt (0.1s)
        m_simState.speed_mm_s = m_simTargetSpeed;
    } else if (m_simState.status == DeviceStatus::MovingBackward) {
        m_simState.position_mm -= (m_simTargetSpeed * 0.1);
        m_simState.speed_mm_s = m_simTargetSpeed;
        if (m_simState.position_mm < 0) m_simState.position_mm = 0;
    } else {
        m_simState.speed_mm_s = 0;
    }
    
    // 发送仿真反馈数据
    emit feedbackReceived(m_simState);
}