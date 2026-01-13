#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QThread>
#include "protocol.h"

// 仿真模式开关：注释掉此行则编译为真实串口模式
// 在没有真实硬件连接时，使用仿真模式测试逻辑
// #define SIMULATION_MODE

/**
 * @brief 串口通信管理类
 * 
 * 负责与下位机进行串口通信。
 * 支持两种模式（混合运行时）：
 * 1. 真实模式：当选择物理端口（如 COM1, COM3）时，通过 QSerialPort 读写。
 * 2. 仿真模式：当选择 "COM_VIRTUAL" 端口时，模拟设备响应，用于开发和测试。
 * 
 * 该类通常运行在独立的工作线程中，通过信号槽与 DeviceController 通信。
 */
class SerialManager : public QObject
{
    Q_OBJECT
public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager();

public slots:
    // --- 控制指令槽函数 (通常在子线程执行) ---

    /**
     * @brief 打开串口
     * @param portName 端口名称 (e.g., "COM1" or "COM_VIRTUAL")
     * @param baudRate 波特率 (e.g., 115200)
     */
    void openPort(const QString &portName, int baudRate);

    /**
     * @brief 关闭串口
     */
    void closePort();

    /**
     * @brief 处理并发送控制指令
     * @param cmd 控制指令结构体
     */
    void processCommand(ControlCommand cmd);

signals:
    // --- 状态反馈信号 ---

    /**
     * @brief 串口打开结果信号
     * @param success true表示成功，false表示失败
     */
    void portOpened(bool success);

    /**
     * @brief 连接错误信号
     * @param msg 错误描述信息
     */
    void connectionError(const QString &msg);

    /**
     * @brief 收到设备反馈数据信号
     * 这是高频信号，用于实时更新 UI 和日志
     * @param data 反馈数据结构体
     */
    void feedbackReceived(MotionFeedback data); 

private slots:
    /**
     * @brief 串口有数据可读时的处理函数 (真实模式)
     */
    void handleReadyRead(); 

    /**
     * @brief 仿真定时器超时处理函数 (仿真模式)
     * 模拟设备的状态更新
     */
    void handleSimTimeout(); 

private:
    bool m_isSimulating = false;     ///< 当前是否处于仿真模式

    QSerialPort *m_serial = nullptr; ///< Qt串口对象
    
    // --- 仿真相关变量 ---
    QTimer *m_simTimer = nullptr;    ///< 仿真定时器
    MotionFeedback m_simState;       ///< 仿真设备的当前状态
    double m_simTargetSpeed = 0.0;   ///< 仿真目标速度
};

#endif // SERIALMANAGER_H