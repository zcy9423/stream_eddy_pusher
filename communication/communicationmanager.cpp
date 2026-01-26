#include "communicationmanager.h"
#include "../utils/logger.h"
#include "../core/configmanager.h"
#include <QHostAddress>
#include <QCoreApplication>

CommunicationManager::CommunicationManager(QObject *parent) : QObject(parent)
{
}

CommunicationManager::~CommunicationManager()
{
    cleanup();
}

void CommunicationManager::cleanup()
{
    if (m_serial) {
        if (m_serial->isOpen()) m_serial->close();
        delete m_serial;
        m_serial = nullptr;
    }
    if (m_tcpSocket) {
        m_tcpSocket->abort();
        delete m_tcpSocket;
        m_tcpSocket = nullptr;
    }
    if (m_simTimer) {
        m_simTimer->stop();
        delete m_simTimer;
        m_simTimer = nullptr;
    }
    m_isConnected = false;
    m_rxBuffer.clear();
}

void CommunicationManager::openConnection(int type, const QString &address, int portOrBaud)
{
    cleanup(); // 先清理旧连接

    m_currentType = static_cast<ConnectionType>(type);

    if (m_currentType == Simulation) {
        LOG_INFO << "启动仿真模式";
        m_simTimer = new QTimer(this);
        connect(m_simTimer, &QTimer::timeout, this, &CommunicationManager::handleSimTimeout);
        
        m_simState.status = DeviceStatus::Idle;
        m_simState.position_mm = 0.0;
        m_simTimer->start(100); // 10Hz
        
        m_isConnected = true;
        emit connectionOpened(true);
    }
    else if (m_currentType == Serial) {
        m_serial = new QSerialPort(this);
        m_serial->setPortName(address);
        m_serial->setBaudRate(portOrBaud);
        
        connect(m_serial, &QSerialPort::readyRead, this, &CommunicationManager::handleSerialReadyRead);
        connect(m_serial, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error){
            if (error != QSerialPort::NoError && m_isConnected) {
                LOG_ERR << "串口错误: " << m_serial->errorString();
                emit connectionError(m_serial->errorString());
                closeConnection();
            }
        });

        if (m_serial->open(QIODevice::ReadWrite)) {
            LOG_INFO << "串口已打开: " << address << " Baud:" << portOrBaud;
            m_isConnected = true;
            emit connectionOpened(true);
        } else {
            LOG_ERR << "打开串口失败: " << m_serial->errorString();
            emit connectionOpened(false);
            emit connectionError(m_serial->errorString());
            cleanup();
        }
    }
    else if (m_currentType == Tcp) {
        m_tcpSocket = new QTcpSocket(this);
        connect(m_tcpSocket, &QTcpSocket::readyRead, this, &CommunicationManager::handleTcpReadyRead);
        connect(m_tcpSocket, &QTcpSocket::connected, this, &CommunicationManager::handleTcpConnected);
        // Qt6 changed error signal, use errorOccurred? Or static cast.
        // QTcpSocket inherits QAbstractSocket
        connect(m_tcpSocket, &QAbstractSocket::errorOccurred, this, &CommunicationManager::handleTcpError);

        LOG_INFO << "正在连接 TCP: " << address << ":" << portOrBaud;
        m_tcpSocket->connectToHost(address, portOrBaud);
        // 异步连接，结果在 handleTcpConnected 或 handleTcpError 中处理
    }
}

void CommunicationManager::closeConnection()
{
    if (!m_isConnected && !m_serial && !m_tcpSocket && !m_simTimer) {
        return;
    }
    const bool wasConnected = m_isConnected;
    cleanup();
    LOG_INFO << "连接已关闭";
    if (wasConnected && !QCoreApplication::closingDown()) {
        emit connectionOpened(false);
    }
}

void CommunicationManager::handleTcpConnected()
{
    LOG_INFO << "TCP 连接成功";
    m_isConnected = true;
    emit connectionOpened(true);
}

void CommunicationManager::handleTcpError()
{
    if (m_tcpSocket) {
        QString err = m_tcpSocket->errorString();
        LOG_ERR << "TCP 错误: " << err;
        emit connectionError(err);
        // 如果是正在连接阶段失败，也需要发 connectionOpened(false)
        if (!m_isConnected) {
            emit connectionOpened(false);
        } else {
            // 如果是运行中断开
            closeConnection();
        }
    }
}

void CommunicationManager::processCommand(ControlCommand cmd)
{
    if (m_currentType == Simulation) {
        // 仿真逻辑
        if (cmd.type == ControlCommand::MoveForward) {
            m_simState.status = DeviceStatus::MovingForward;
            m_simTargetSpeed = cmd.param;
        } else if (cmd.type == ControlCommand::MoveBackward) {
            m_simState.status = DeviceStatus::MovingBackward;
            m_simTargetSpeed = cmd.param;
        } else if (cmd.type == ControlCommand::Stop) {
            m_simState.status = DeviceStatus::Idle;
            m_simTargetSpeed = 0.0;
        } else if (cmd.type == ControlCommand::SetSpeed) {
            // 仅在运动中生效
             if (m_simState.status != DeviceStatus::Idle) {
                 m_simTargetSpeed = cmd.param;
             }
        }
        return;
    }

    QByteArray packet = Protocol::pack(cmd);
    
    if (m_currentType == Serial && m_serial && m_serial->isOpen()) {
        m_serial->write(packet);
    }
    else if (m_currentType == Tcp && m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        m_tcpSocket->write(packet);
        m_tcpSocket->flush();
    }
}

void CommunicationManager::handleSerialReadyRead()
{
    if (!m_serial) return;
    m_rxBuffer.append(m_serial->readAll());
    parseBuffer();
}

void CommunicationManager::handleTcpReadyRead()
{
    if (!m_tcpSocket) return;
    m_rxBuffer.append(m_tcpSocket->readAll());
    parseBuffer();
}

void CommunicationManager::parseBuffer()
{
    // 简单的解包逻辑 (假设 Protocol 类有解包方法，这里暂时手动实现或简化)
    // 实际项目中应使用 Protocol::unpack 或类似的流解析器处理粘包/分包
    
    // 这里简单查找帧头帧尾 (仅作为示例，需与 protocol.h 配合)
    // 假设数据包固定长度为 header(1)+...
    
    // 简易处理：尝试找到一帧完整数据
    // 假设 Protocol 还没提供 unpack，我们先简单模拟
    // 实际上 protocol.h 里没有 unpack，只有 pack。
    // 为了稳健性，这里应该实现一个环形缓冲解析，但为了演示，这里假设一次收发完整
    
    // 临时逻辑：如果缓冲区有足够数据，且包含帧头帧尾
    while (m_rxBuffer.size() >= 15) { // 最小长度估计
        int headIdx = m_rxBuffer.indexOf((char)FRAME_HEADER);
        if (headIdx == -1) {
            m_rxBuffer.clear();
            break;
        }
        
        // 丢弃头部之前的垃圾数据
        if (headIdx > 0) {
            m_rxBuffer.remove(0, headIdx);
        }
        
        // 检查是否有足够长度 (Command packet is small, Feedback might be larger)
        // Feedback structure: [Header] [Type] [Len] [Payload...] [Checksum] [Footer]
        // 这里需要 Protocol 定义 Feedback 的封包格式。
        // 由于 protocol.h 中没有 define feedback packet layout，我们这里假设接收到的是 MotionFeedback 结构体的直接二进制 (不推荐) 
        // 或者我们假设下位机发送格式与 Command 类似。
        
        // 既然这是一个 Mock 实现，我们先假设接收到了数据并解析
        // TODO: 完善 Protocol::unpackFeedback
        
        // 模拟解析成功
        MotionFeedback fb;
        // 简单从 buffer 模拟提取 (实际需按协议解包)
        // 此处为了兼容性，暂时不深入实现解包细节，而是假设 buffer 里就是 raw struct (仅作演示)
        if (m_rxBuffer.size() >= (int)sizeof(MotionFeedback)) {
             // 实际上这行不通，因为有字节对齐和端序问题。
             // 我们假设下位机发来的是标准协议包。
             // 暂时略过详细解包，直接清空 buffer
             m_rxBuffer.clear();
        } else {
            break; // 数据不够
        }
    }
}

void CommunicationManager::handleSimTimeout()
{
    // 简单的运动学模拟
    double dt = 0.1; // 100ms
    
    if (m_simState.status == DeviceStatus::MovingForward) {
        m_simState.speed_mm_s = m_simTargetSpeed;
        m_simState.position_mm += m_simState.speed_mm_s * dt;
    } 
    else if (m_simState.status == DeviceStatus::MovingBackward) {
        m_simState.speed_mm_s = m_simTargetSpeed;
        m_simState.position_mm -= m_simState.speed_mm_s * dt;
    }
    else {
        m_simState.speed_mm_s = 0.0;
    }
    
    // 模拟限位 - 使用配置中的实际限位值
    double maxPos = ConfigManager::instance().maxPosition();
    if (m_simState.position_mm >= maxPos) {
        m_simState.position_mm = maxPos;
        m_simState.rightLimit = true;
        // m_simState.status = DeviceStatus::Idle; // 碰到限位停止？
    } else {
        m_simState.rightLimit = false;
    }
    
    if (m_simState.position_mm <= 0.0) {
        m_simState.position_mm = 0.0;
        m_simState.leftLimit = true;
    } else {
        m_simState.leftLimit = false;
    }
    
    emit feedbackReceived(m_simState);
}
