#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

/**
 * @brief 协议定义头文件
 * 
 * 包含设备通信所需的状态枚举、数据结构定义。
 * 这些结构体需要在 Qt 元对象系统中注册以支持跨线程信号槽传递。
 */

/**
 * @brief 设备运行状态枚举
 */
enum class DeviceStatus {
    Unknown,        ///< 未知状态
    Idle,           ///< 空闲/停止
    MovingForward,  ///< 正在向前推进
    MovingBackward, ///< 正在向后拉回
    Error           ///< 故障状态
};

/**
 * @brief 运动反馈数据结构
 * 包含设备实时上报的位置、速度和状态信息。
 */
struct MotionFeedback {
    double position_mm = 0.0;             ///< 当前绝对位置 (单位: mm)
    double speed_mm_s = 0.0;              ///< 当前实时速度 (单位: mm/s)
    DeviceStatus status = DeviceStatus::Unknown; ///< 当前设备运行状态
    uint32_t errorCode = 0;               ///< 错误码 (0表示无错误)
};

/**
 * @brief 控制指令数据结构
 * 用于从上位机向设备发送控制命令。
 */
struct ControlCommand {
    /**
     * @brief 指令类型枚举
     */
    enum Type { 
        Stop,           ///< 停止运动
        MoveForward,    ///< 向前移动
        MoveBackward,   ///< 向后移动
        SetSpeed        ///< 设置速度
    };
    
    Type type;          ///< 指令类型
    double param = 0.0; ///< 指令参数 (如速度值或距离值)
};

#endif // PROTOCOL_H