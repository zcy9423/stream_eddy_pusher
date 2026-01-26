#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QTimer>
#include "../communication/protocol.h"

/**
 * @brief 自动任务管理器类
 * 
 * 负责执行复杂的自动控制逻辑，例如在指定区间内往返扫描。
 * 
 * 设计理念：
 * - 纯逻辑层：不直接操作 UI 或 硬件。
 * - 信号驱动：通过发送 requestMove/requestStop 信号请求底层动作。
 * - 状态机：内部维护状态机 (Idle, AutoForward, AutoBackward, etc.)。
 * 
 * 使用方式：
 *   TaskManager* tm = new TaskManager(this);
 *   connect(tm, &TaskManager::requestMoveForward, dc, &DeviceController::manualMove...);
 *   connect(dc, &DeviceController::deviceStateUpdated, tm, &TaskManager::onPositionUpdated...);
 */
class TaskManager final : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 任务状态枚举
     */
    enum class State {
        Idle,           ///< 空闲
        AutoForward,    ///< 自动向前移动中
        AutoBackward,   ///< 自动向后移动中
        Paused,         ///< 暂停状态
        Stopping,       ///< 正在停止
        Fault,          ///< 故障状态
        StepExecution,  ///< 脚本/配方执行中
        Resetting       ///< 重置中（回到初始位置）
    };
    Q_ENUM(State)

    /**
     * @brief 任务步骤类型
     */
    enum class StepType {
        MoveTo,     ///< 移动到绝对位置
        Wait,       ///< 等待一段时间
        SetSpeed    ///< 设置速度
    };
    Q_ENUM(StepType)

    /**
     * @brief 任务步骤结构体
     */
    struct TaskStep {
        StepType type;
        double param1 = 0.0; // MoveTo: targetPos; Wait: ms; SetSpeed: speed
        double param2 = 0.0; // MoveTo: speed (optional)
        QString description;
    };

    explicit TaskManager(QObject* parent = nullptr);

    /**
     * @brief 启动高级任务序列 (脚本化控制)
     * @param steps 步骤列表
     * @param cycles 循环执行次数 (<=0 无限)
     */
    Q_INVOKABLE void startTaskSequence(const QList<TaskStep> &steps, int cycles = 1);

    /**
     * @brief 启动自动往返扫描任务
     * 
     * @param minPos 最小位置 (mm)
     * @param maxPos 最大位置 (mm)
     * @param speed 扫描速度 (mm/s)
     * @param cycles 往返次数 (一次往返 = max -> min 或 min -> max -> min)，<=0 表示无限循环
     */
    Q_INVOKABLE void startAutoScan(double minPos, double maxPos, double speed, int cycles = 1);

    /**
     * @brief 暂停当前任务
     * 发送停止请求，并记录当前状态以便恢复。
     */
    Q_INVOKABLE void pause();

    /**
     * @brief 恢复暂停的任务
     * 从暂停前的状态继续运行。
     */
    Q_INVOKABLE void resume();

    /**
     * @brief 停止所有任务
     * 立即终止自动流程并重置状态为 Idle。
     */
    Q_INVOKABLE void stopAll();

    /**
     * @brief 重置任务
     * 只能在暂停状态下调用，重置任务状态并回到初始位置
     */
    Q_INVOKABLE void resetTask();

    /**
     * @brief 设置到位判断的容差值
     * @param tol 容差 (mm)，默认 0.2
     */
    void setPositionTolerance(double tol);
    double positionTolerance() const;

    /**
     * @brief 设置边界超时时间
     * 若在指定时间内未到达目标边界，视为故障。
     * @param ms 毫秒数，默认 30s
     */
    void setEdgeTimeoutMs(int ms);
    int edgeTimeoutMs() const;

    State state() const { return m_state; }
    
    /**
     * @brief 检查是否处于运行状态
     * @return true 如果正在自动运行或停止中
     */
    bool isRunning() const;

signals:
    // --- 向下层发送的控制请求 ---
    
    void requestMoveForward(double speed);
    void requestMoveBackward(double speed);
    void requestStop();

    // --- 向上层发送的状态反馈 ---
    
    void stateChanged(TaskManager::State state);
    void progressChanged(int completedCycles, int targetCycles); // targetCycles <=0 表示无限
    void message(const QString& text);
    void fault(const QString& reason);
    void taskCompleted(); // 任务完成信号
    void taskFailed(const QString& reason); // 任务失败信号

public slots:
    /**
     * @brief 接收位置更新
     * 由外部（如 DeviceController）调用，用于驱动状态机跳转。
     * @param position 当前绝对位置
     */
    void onPositionUpdated(double position);
    
    /**
     * @brief 接收完整的运动反馈
     * @param fb 运动反馈数据
     */
    void updateFeedback(const MotionFeedback &fb);

private slots:
    /**
     * @brief 看门狗定时器回调
     * 检查任务执行是否超时。
     */
    void onWatchdogTick();

private:
    void setState(State s);
    void enterFault(const QString& reason);

    void startMovingToMax();
    void startMovingToMin();

    // 判断是否到达目标位置 (在容差范围内)
    bool reached(double pos, double target) const;

    // --- 序列执行相关 ---
    void executeNextStep();
    void executeStep(const TaskStep &step);
    void checkStepCompletion(double currentPos);

private:
    State   m_state {State::Idle};
    State   m_lastMotionState {State::Idle};

    // 自动扫描参数 (Legacy)
    double  m_minPos {0.0};
    double  m_maxPos {0.0};
    double  m_speed  {1.0};

    // 通用参数
    int     m_targetCycles {1};   // <=0 infinite
    int     m_completedCycles {0};

    // 序列任务参数
    QList<TaskStep> m_sequenceSteps;
    int m_currentStepIndex {0};
    double m_currentStepTargetPos {0.0};
    bool m_isStepWaiting {false};
    qint64 m_waitStartTime {0};
    int m_waitDurationMs {0};

    double  m_position {0.0};
    double  m_tol {0.2};          // 到位容差
    double  m_resetTargetPos {0.0}; // 重置目标位置
    
    // 超时检测相关
    QTimer  m_watchdog;
    qint64  m_motionStartMs {0};
    int     m_edgeTimeoutMs {30000}; // 30s
};

#endif // TASKMANAGER_H
