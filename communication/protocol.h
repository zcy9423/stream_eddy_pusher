#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <QByteArray>
#include <QDataStream>
#include <QIODevice>

/**
 * @brief 协议定义头文件
 * 
 * 包含设备通信所需的状态枚举、数据结构定义。
 * 以及协议的打包和解包功能。
 */

// --- 协议常量定义 ---
const uint8_t FRAME_HEADER = 0xAA;
const uint8_t FRAME_FOOTER = 0x55;

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
    
    // --- 新增硬件信号 ---
    bool leftLimit = false;  ///< 左限位触发 (True=触发)
    bool rightLimit = false; ///< 右限位触发 (True=触发)
    bool emergencyStop = false; ///< 急停按下
    bool overCurrent = false;   ///< 过流报警
    bool stalled = false;       ///< 堵转报警
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
        Stop = 0x01,           ///< 停止运动
        MoveForward = 0x02,    ///< 向前移动
        MoveBackward = 0x03,   ///< 向后移动
        SetSpeed = 0x04        ///< 设置速度
    };
    
    Type type;          ///< 指令类型
    double param = 0.0; ///< 指令参数 (如速度值或距离值)
};

/**
 * @brief 协议处理类
 * 提供静态方法用于数据的打包和解包。
 */
class Protocol {
public:
    /**
     * @brief 打包控制指令
     * 格式: [Header(1)] [Cmd(1)] [Len(4)] [Param(8)] [Checksum(1)] [Footer(1)]
     */
    static QByteArray pack(const ControlCommand &cmd) {
        QByteArray packet;
        QDataStream stream(&packet, QIODevice::WriteOnly);
        stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
        stream.setByteOrder(QDataStream::LittleEndian); // 使用小端序

        // 1. 帧头
        stream << (uint8_t)FRAME_HEADER;
        
        // 2. 命令字
        stream << (uint8_t)cmd.type;
        
        // 3. 数据长度 (目前固定 param 为 double 8字节)
        stream << (uint32_t)sizeof(double);
        
        // 4. 参数数据
        stream << cmd.param;
        
        // 5. 计算校验和 (从 Header 到 Data)
        uint8_t checksum = 0;
        for (char c : packet) {
            checksum += (uint8_t)c;
        }
        stream << checksum;
        
        // 6. 帧尾
        stream << (uint8_t)FRAME_FOOTER;
        
        return packet;
    }

    /**
     * @brief 尝试从缓冲区解析一帧数据
     * @param buffer 输入/输出缓冲区，解析成功后会移除已处理的数据
     * @param outFeedback 输出参数，解析出的反馈数据
     * @return true 解析成功，false 数据不足或校验失败
     */
    static bool parse(QByteArray &buffer, MotionFeedback &outFeedback) {
        // 最小帧长度: Header(1) + Status(1) + Pos(8) + Speed(8) + Error(4) + Flags(1) + Checksum(1) + Footer(1) = 25字节
        const int MIN_FRAME_SIZE = 25;
        
        while (buffer.size() >= MIN_FRAME_SIZE) {
            // 1. 寻找帧头
            int headerIndex = buffer.indexOf((char)FRAME_HEADER);
            if (headerIndex == -1) {
                buffer.clear(); // 没有帧头，丢弃所有数据
                return false;
            }
            
            // 丢弃帧头前面的垃圾数据
            if (headerIndex > 0) {
                buffer.remove(0, headerIndex);
            }
            
            // 再次检查长度
            if (buffer.size() < MIN_FRAME_SIZE) return false;
            
            // 2. 检查帧尾
            if ((uint8_t)buffer[MIN_FRAME_SIZE - 1] != FRAME_FOOTER) {
                // 帧尾不对，移除当前帧头，继续寻找下一个
                buffer.remove(0, 1);
                continue;
            }
            
            // 3. 校验和验证
            uint8_t calcSum = 0;
            for (int i = 0; i < MIN_FRAME_SIZE - 2; i++) {
                calcSum += (uint8_t)buffer[i];
            }
            if (calcSum != (uint8_t)buffer[MIN_FRAME_SIZE - 2]) {
                // 校验失败
                buffer.remove(0, 1);
                continue;
            }
            
            // 4. 解析数据
            QDataStream stream(buffer);
            stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
            stream.setByteOrder(QDataStream::LittleEndian);
            
            uint8_t header, statusByte, footer, checksum, flags;
            uint32_t errorCode;
            double pos, speed;
            
            // 读取Flags字节 (假设放在 statusByte 后面，或者在协议末尾增加)
            // 这里为了演示，我们假设协议升级了，在 ErrorCode 后面增加了一个 Flags 字节
            // Header(1) + Status(1) + Pos(8) + Speed(8) + Error(4) + Flags(1) + Checksum(1) + Footer(1) = 25字节
            // 如果保持 MIN_FRAME_SIZE = 24，则需要重构协议，或者复用 ErrorCode。
            // 简单起见，我们暂且认为 Flags 包含在 ErrorCode 的高位，或者 StatusByte 的高位
            // 为了更清晰，我们扩展协议长度到 25 字节
            
            stream >> header >> statusByte >> pos >> speed >> errorCode >> flags >> checksum >> footer;
            
            outFeedback.position_mm = pos;
            outFeedback.speed_mm_s = speed;
            outFeedback.errorCode = errorCode;
            
            // 解析 Flags
            outFeedback.leftLimit = (flags & 0x01);
            outFeedback.rightLimit = (flags & 0x02);
            outFeedback.emergencyStop = (flags & 0x04);
            outFeedback.overCurrent = (flags & 0x08);
            outFeedback.stalled = (flags & 0x10);
            
            switch (statusByte) {
                case 0: outFeedback.status = DeviceStatus::Idle; break;
                case 1: outFeedback.status = DeviceStatus::MovingForward; break;
                case 2: outFeedback.status = DeviceStatus::MovingBackward; break;
                case 3: outFeedback.status = DeviceStatus::Error; break;
                default: outFeedback.status = DeviceStatus::Unknown; break;
            }
            
            // 移除已处理的帧
            buffer.remove(0, MIN_FRAME_SIZE);
            return true;
        }
        
        return false;
    }
};

#endif // PROTOCOL_H