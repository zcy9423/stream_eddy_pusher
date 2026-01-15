#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QTcpSocket>
#include <QTimer>
#include <QThread>
#include "protocol.h"

/**
 * @brief 通信管理类
 * 
 * 负责与下位机进行通信，支持多种模式：
 * 1. 串口模式 (Serial): 通过 QSerialPort 连接
 * 2. 网络模式 (TCP): 通过 QTcpSocket 连接
 * 3. 仿真模式 (Simulation): 模拟设备响应
 */
class CommunicationManager : public QObject
{
    Q_OBJECT
public:
    enum ConnectionType {
        Serial,
        Tcp,
        Simulation
    };
    Q_ENUM(ConnectionType)

    explicit CommunicationManager(QObject *parent = nullptr);
    ~CommunicationManager();

public slots:
    /**
     * @brief 建立连接
     * @param type 连接类型 (0=Serial, 1=Tcp, 2=Simulation)
     * @param address 地址 (串口名 或 IP地址)
     * @param portOrBaud 端口参数 (波特率 或 TCP端口号)
     */
    void openConnection(int type, const QString &address, int portOrBaud);

    /**
     * @brief 断开连接
     */
    void closeConnection();

    /**
     * @brief 处理并发送控制指令
     */
    void processCommand(ControlCommand cmd);

signals:
    void connectionOpened(bool success);
    void connectionError(const QString &msg);
    void feedbackReceived(MotionFeedback data);

private slots:
    void handleSerialReadyRead();
    void handleTcpReadyRead();
    void handleTcpConnected();
    void handleTcpError(); // 简化处理
    void handleSimTimeout();

private:
    void cleanup();
    void parseBuffer();

    ConnectionType m_currentType = ConnectionType::Serial;
    bool m_isConnected = false;

    // Serial
    QSerialPort *m_serial = nullptr;
    
    // TCP
    QTcpSocket *m_tcpSocket = nullptr;

    // Buffer
    QByteArray m_rxBuffer;

    // Simulation
    QTimer *m_simTimer = nullptr;
    MotionFeedback m_simState;
    double m_simTargetSpeed = 0.0;
};

#endif // COMMUNICATIONMANAGER_H
