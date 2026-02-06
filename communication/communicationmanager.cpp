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
    LOG_INFO << "========== 开始建立连接 ==========";
    LOG_INFO << "连接类型: " << type << " (0=Serial, 1=TCP, 2=Simulation)";
    LOG_INFO << "地址/端口名: " << address;
    LOG_INFO << "波特率/端口号: " << portOrBaud;
    
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
        LOG_INFO << "仿真模式启动成功，定时器频率: 10Hz";
        emit connectionOpened(true);
    }
    else if (m_currentType == Serial) {
        LOG_INFO << "准备打开串口连接";
        m_serial = new QSerialPort(this);
        m_serial->setPortName(address);
        m_serial->setBaudRate(portOrBaud);
        LOG_INFO << "串口参数设置完成 - 端口: " << address << ", 波特率: " << portOrBaud;
        
        connect(m_serial, &QSerialPort::readyRead, this, &CommunicationManager::handleSerialReadyRead);
        connect(m_serial, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error){
            if (error != QSerialPort::NoError && m_isConnected) {
                QString errorMsg = getSerialErrorMessage(error);
                LOG_ERR << "串口运行时错误: " << errorMsg << " (错误代码: " << error << ")";
                emit connectionError(errorMsg);
                closeConnection();
            }
        });

        if (m_serial->open(QIODevice::ReadWrite)) {
            LOG_INFO << "串口已成功打开: " << address << " Baud:" << portOrBaud;
            m_isConnected = true;
            emit connectionOpened(true);
        } else {
            QString errorMsg = getSerialErrorMessage(m_serial->error());
            LOG_ERR << "打开串口失败: " << errorMsg << " (错误代码: " << m_serial->error() << ")";
            emit connectionOpened(false);
            emit connectionError(errorMsg);
            cleanup();
        }
    }
    else if (m_currentType == Tcp) {
        LOG_INFO << "准备建立TCP连接";
        m_tcpSocket = new QTcpSocket(this);
        connect(m_tcpSocket, &QTcpSocket::readyRead, this, &CommunicationManager::handleTcpReadyRead);
        connect(m_tcpSocket, &QTcpSocket::connected, this, &CommunicationManager::handleTcpConnected);
        // Qt6 changed error signal, use errorOccurred? Or static cast.
        // QTcpSocket inherits QAbstractSocket
        connect(m_tcpSocket, &QAbstractSocket::errorOccurred, this, &CommunicationManager::handleTcpError);

        LOG_INFO << "正在连接 TCP: " << address << ":" << portOrBaud;
        m_tcpSocket->connectToHost(address, portOrBaud);
        LOG_INFO << "TCP连接请求已发送，等待响应...";
        // 异步连接，结果在 handleTcpConnected 或 handleTcpError 中处理
    }
}

void CommunicationManager::closeConnection()
{
    if (!m_isConnected && !m_serial && !m_tcpSocket && !m_simTimer) {
        LOG_INFO << "closeConnection: 无活动连接，跳过关闭操作";
        return;
    }
    LOG_INFO << "========== 开始关闭连接 ==========";
    const bool wasConnected = m_isConnected;
    cleanup();
    LOG_INFO << "连接已关闭，资源已清理";
    if (wasConnected && !QCoreApplication::closingDown()) {
        emit connectionOpened(false);
    }
}

void CommunicationManager::handleTcpConnected()
{
    LOG_INFO << "TCP 连接成功建立";
    m_isConnected = true;
    emit connectionOpened(true);
}

void CommunicationManager::handleTcpError()
{
    if (m_tcpSocket) {
        QString errorMsg = getTcpErrorMessage(m_tcpSocket->error());
        LOG_ERR << "TCP 错误: " << errorMsg << " (错误代码: " << m_tcpSocket->error() << ")";
        emit connectionError(errorMsg);
        // 如果是正在连接阶段失败，也需要发 connectionOpened(false)
        if (!m_isConnected) {
            LOG_INFO << "TCP连接建立失败";
            emit connectionOpened(false);
        } else {
            // 如果是运行中断开
            LOG_INFO << "TCP运行时断开连接";
            closeConnection();
        }
    }
}

void CommunicationManager::processCommand(ControlCommand cmd)
{
    LOG_INFO << "处理控制指令 - 类型: " << cmd.type << ", 参数: " << cmd.param;
    
    if (m_currentType == Simulation) {
        // 仿真逻辑
        if (cmd.type == ControlCommand::MoveForward) {
            m_simState.status = DeviceStatus::MovingForward;
            m_simTargetSpeed = cmd.param;
            LOG_INFO << "仿真: 开始向前移动，目标速度: " << cmd.param << " mm/s";
        } else if (cmd.type == ControlCommand::MoveBackward) {
            m_simState.status = DeviceStatus::MovingBackward;
            m_simTargetSpeed = cmd.param;
            LOG_INFO << "仿真: 开始向后移动，目标速度: " << cmd.param << " mm/s";
        } else if (cmd.type == ControlCommand::Stop) {
            m_simState.status = DeviceStatus::Idle;
            m_simTargetSpeed = 0.0;
            LOG_INFO << "仿真: 停止运动";
        } else if (cmd.type == ControlCommand::SetSpeed) {
            // 仅在运动中生效
             if (m_simState.status != DeviceStatus::Idle) {
                 m_simTargetSpeed = cmd.param;
                 LOG_INFO << "仿真: 设置速度为: " << cmd.param << " mm/s";
             } else {
                 LOG_INFO << "仿真: 设备空闲，忽略速度设置指令";
             }
        }
        return;
    }

    QByteArray packet = Protocol::pack(cmd);
    LOG_INFO << "指令已打包，数据包大小: " << packet.size() << " 字节";
    
    if (m_currentType == Serial && m_serial && m_serial->isOpen()) {
        qint64 written = m_serial->write(packet);
        LOG_INFO << "串口发送: " << written << " 字节";
    }
    else if (m_currentType == Tcp && m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        qint64 written = m_tcpSocket->write(packet);
        m_tcpSocket->flush();
        LOG_INFO << "TCP发送: " << written << " 字节";
    } else {
        LOG_WARN << "无法发送指令: 设备未连接或连接状态异常";
    }
}

void CommunicationManager::handleSerialReadyRead()
{
    if (!m_serial) return;
    QByteArray data = m_serial->readAll();
    LOG_INFO << "串口接收数据: " << data.size() << " 字节";
    m_rxBuffer.append(data);
    parseBuffer();
}

void CommunicationManager::handleTcpReadyRead()
{
    if (!m_tcpSocket) return;
    QByteArray data = m_tcpSocket->readAll();
    LOG_INFO << "TCP接收数据: " << data.size() << " 字节";
    m_rxBuffer.append(data);
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

QString CommunicationManager::getSerialErrorMessage(QSerialPort::SerialPortError error)
{
    switch (error) {
        case QSerialPort::NoError:
            return "无错误";
        case QSerialPort::DeviceNotFoundError:
            return "串口设备未找到，请检查设备是否已连接";
        case QSerialPort::PermissionError:
            return "串口访问权限被拒绝，请检查设备是否被其他程序占用";
        case QSerialPort::OpenError:
            return "无法打开串口，设备可能已被占用或不存在";
        case QSerialPort::NotOpenError:
            return "串口未打开";
        case QSerialPort::WriteError:
            return "串口写入失败";
        case QSerialPort::ReadError:
            return "串口读取失败";
        case QSerialPort::ResourceError:
            return "串口资源错误，设备可能已断开连接";
        case QSerialPort::UnsupportedOperationError:
            return "串口不支持此操作";
        case QSerialPort::TimeoutError:
            return "串口操作超时";
        default:
            return QString("串口错误 (代码: %1)").arg(error);
    }
}

QString CommunicationManager::getTcpErrorMessage(QAbstractSocket::SocketError error)
{
    switch (error) {
        case QAbstractSocket::ConnectionRefusedError:
            return "TCP连接被拒绝，请检查目标设备是否开启服务";
        case QAbstractSocket::RemoteHostClosedError:
            return "远程主机关闭了连接";
        case QAbstractSocket::HostNotFoundError:
            return "找不到主机，请检查IP地址是否正确";
        case QAbstractSocket::SocketAccessError:
            return "网络访问权限被拒绝";
        case QAbstractSocket::SocketResourceError:
            return "网络资源不足";
        case QAbstractSocket::SocketTimeoutError:
            return "TCP连接超时，请检查网络连接和目标设备";
        case QAbstractSocket::NetworkError:
            return "网络错误，请检查网络连接";
        case QAbstractSocket::AddressInUseError:
            return "地址已被占用";
        case QAbstractSocket::SocketAddressNotAvailableError:
            return "地址不可用";
        case QAbstractSocket::UnsupportedSocketOperationError:
            return "不支持的网络操作";
        case QAbstractSocket::ProxyAuthenticationRequiredError:
            return "代理服务器需要身份验证";
        case QAbstractSocket::SslHandshakeFailedError:
            return "SSL握手失败";
        case QAbstractSocket::UnfinishedSocketOperationError:
            return "网络操作未完成";
        case QAbstractSocket::ProxyConnectionRefusedError:
            return "代理服务器拒绝连接";
        case QAbstractSocket::ProxyConnectionClosedError:
            return "代理服务器关闭了连接";
        case QAbstractSocket::ProxyConnectionTimeoutError:
            return "代理服务器连接超时";
        case QAbstractSocket::ProxyNotFoundError:
            return "找不到代理服务器";
        case QAbstractSocket::ProxyProtocolError:
            return "代理协议错误";
        default:
            return QString("TCP错误 (代码: %1)").arg(error);
    }
}
