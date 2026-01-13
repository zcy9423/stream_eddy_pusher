#ifndef DEVICECONTROLLER_H
#define DEVICECONTROLLER_H

#include <QObject>
#include <QThread>
#include "../communication/serialmanager.h"
#include "../communication/protocol.h"
#include "../data/datamanager.h"

/**
 * @brief 设备控制器类
 * 
 * 作为系统的核心中枢，负责协调 UI 层、通信层 (SerialManager) 和数据层 (DataManager)。
 * 它拥有一个工作线程，将 SerialManager 移动到该线程中运行，以避免阻塞主 UI 线程。
 */
class DeviceController : public QObject
{
    Q_OBJECT
public:
    explicit DeviceController(QObject *parent = nullptr);
    ~DeviceController();

    /**
     * @brief 初始化控制器
     * 启动工作线程，初始化数据库，并建立内部信号槽连接。
     */
    void init();

public slots:
    // --- UI 调用的高层指令 ---

    /**
     * @brief 请求连接设备
     * @param port 端口名
     * @param baud 波特率
     */
    void requestConnect(const QString &port, int baud);

    /**
     * @brief 请求断开连接
     */
    void requestDisconnect();

    /**
     * @brief 手动运动控制
     * @param forward true为前进，false为后退
     * @param speed 速度值
     */
    void manualMove(bool forward, double speed);

    /**
     * @brief 停止运动
     */
    void stopMotion();

    /**
     * @brief 设置速度
     * @param speed 速度值
     */
    void setSpeed(double speed);

signals:
    // --- 向下层 (通信层) 发送的指令 ---
    // 通过信号槽机制跨线程调用 SerialManager 的方法

    /**
     * @brief 命令：打开串口
     */
    void cmdOpenPort(const QString &port, int baud);

    /**
     * @brief 命令：关闭串口
     */
    void cmdClosePort();

    /**
     * @brief 命令：发送控制指令包
     */
    void cmdSendPacket(ControlCommand cmd);

    // --- 向上层 (UI) 反馈的状态 ---

    /**
     * @brief 连接状态改变通知
     * @param connected true已连接，false已断开
     */
    void connectionChanged(bool connected);

    /**
     * @brief 设备实时状态更新通知
     * @param fb 包含位置、速度等信息的反馈包
     */
    void deviceStateUpdated(MotionFeedback fb);

    /**
     * @brief 错误消息通知
     * @param msg 错误描述
     */
    void errorMessage(QString msg);

private slots:
    /**
     * @brief 处理从 SerialManager 接收到的反馈数据
     * @param fb 反馈数据
     */
    void onFeedbackReceived(MotionFeedback fb);

private:
    QThread m_workerThread;         ///< 负责通信的后台工作线程
    SerialManager *m_serialManager; ///< 串口管理器实例
    DataManager *m_dataManager;     ///< 数据管理器实例
};

#endif // DEVICECONTROLLER_H