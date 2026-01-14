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
        // 降低日志频率：仅在非位置更新命令时记录，或移除频繁日志
        if (cmd.type != ControlCommand::SetSpeed) {
             LOG_INFO << "[Sim] 接收命令类型：" << cmd.type << " 参数：" << cmd.param;
        }
    } 
    else {
        // --- 真实模式逻辑 ---
        if (!m_serial || !m_serial->isOpen()) return;
        
        // 使用 Protocol::pack 将结构体转换为字节流
        QByteArray packet = Protocol::pack(cmd); 
        m_serial->write(packet);
        
        // 降低日志频率
        // LOG_INFO << "[Real] 发送数据包，大小：" << packet.size() 
        //          << "命令：" << cmd.type << " 参数：" << cmd.param;
    }
}

/**
 * @brief 处理真实串口数据接收
 */
void SerialManager::handleReadyRead()
{
    // 真实数据解析逻辑
    if (!m_serial) return;
    
    // 1. 读取所有新到达的数据追加到缓冲区
    QByteArray data = m_serial->readAll();
    m_rxBuffer.append(data);
    
    // 2. 循环尝试解析，直到缓冲区数据不足或无有效帧
    // Protocol::parse 内部会自动移除已成功解析的数据
    MotionFeedback fb;
    while (Protocol::parse(m_rxBuffer, fb)) {
        // 解析成功一帧，发送信号通知上层
        emit feedbackReceived(fb);
    }
    
    // 3. (可选) 防止缓冲区无限增长：如果堆积太多垃圾数据，强制清空
    if (m_rxBuffer.size() > 4096) {
        LOG_WARN << "接收缓冲区溢出，丢弃数据。";
        m_rxBuffer.clear();
    }
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