#include "taskmanager.h"

#include <QDateTime>
#include <QtMath>
#include <QDebug>

#include "../communication/protocol.h"
#include "../utils/logger.h"
#include "configmanager.h"

// 获取当前时间戳（毫秒）的辅助函数
static qint64 nowMs()
{
    return QDateTime::currentMSecsSinceEpoch();
}

TaskManager::TaskManager(QObject* parent)
    : QObject(parent)
{
    // 初始化超时参数
    m_edgeTimeoutMs = ConfigManager::instance().motionTimeout();

    // 配置看门狗定时器，用于检测运动超时
    m_watchdog.setInterval(100); // 10Hz 足够用于超时检测
    connect(&m_watchdog, &QTimer::timeout, this, &TaskManager::onWatchdogTick);
}

/**
 * @brief 判断任务管理器是否处于活跃状态
 */
bool TaskManager::isRunning() const
{
    return m_state == State::AutoForward
        || m_state == State::AutoBackward
        || m_state == State::Stopping
        || m_state == State::StepExecution
        || m_state == State::Resetting;
}

void TaskManager::setPositionTolerance(double tol)
{
    m_tol = qMax(0.0, tol);
}

double TaskManager::positionTolerance() const
{
    return m_tol;
}

void TaskManager::setEdgeTimeoutMs(int ms)
{
    m_edgeTimeoutMs = qMax(1000, ms);
}

int TaskManager::edgeTimeoutMs() const
{
    return m_edgeTimeoutMs;
}

/**
 * @brief 启动自动扫描任务
 * 
 * 1. 校验参数合法性。
 * 2. 初始化任务变量。
 * 3. 决定初始运动方向（离哪边远就往哪边跑，或者固定策略）。
 * 4. 启动看门狗并发送运动指令。
 */
void TaskManager::startAutoScan(double minPos, double maxPos, double speed, int cycles)
{
    LOG_INFO << "========== 启动自动扫描任务 ==========";
    LOG_INFO << "参数 - 最小位置: " << minPos << " mm, 最大位置: " << maxPos << " mm";
    LOG_INFO << "参数 - 速度: " << speed << " mm/s, 周期数: " << cycles;
    
    if (m_state != State::Idle && m_state != State::Fault) {
        LOG_WARN << "任务启动失败: 当前状态不是Idle或Fault (当前状态: " << (int)m_state << ")";
        emit message("任务正在运行，请先停止");
        return;
    }

    // 参数校验
    if (qIsNaN(minPos) || qIsNaN(maxPos) || qIsNaN(speed)) {
        LOG_ERR << "参数校验失败: 存在NaN值";
        enterFault("startAutoScan: parameter is NaN.");
        return;
    }

    // 参数校验：检查最大行程限制
    double limitPos = ConfigManager::instance().maxPosition();
    if (maxPos > limitPos) {
        LOG_ERR << "参数校验失败: 目标位置 " << maxPos << " mm 超过系统限制 " << limitPos << " mm";
        emit fault(QString("目标位置 %1 mm 超过系统最大行程限制 %2 mm").arg(maxPos).arg(limitPos));
        return;
    }
    
    // 参数校验：检查最大速度限制
    double limitSpeed = ConfigManager::instance().maxSpeed();
    if (speed > limitSpeed) {
        LOG_ERR << "参数校验失败: 目标速度 " << speed << " mm/s 超过系统限制 " << limitSpeed << " mm/s";
        emit fault(QString("目标速度 %1 mm/s 超过系统最大速度限制 %2 mm/s").arg(speed).arg(limitSpeed));
        return;
    }

    if (maxPos <= minPos) {
        LOG_ERR << "参数校验失败: maxPos <= minPos";
        enterFault("startAutoScan: maxPos must be greater than minPos.");
        return;
    }
    if (speed <= 0.0) {
        LOG_ERR << "参数校验失败: speed <= 0";
        enterFault("startAutoScan: speed must be > 0.");
        return;
    }

    LOG_INFO << "参数校验通过";
    
    m_minPos = minPos;
    m_maxPos = maxPos;
    m_speed  = speed;
    m_targetCycles = cycles;
    m_completedCycles = 0;

    emit progressChanged(m_completedCycles, m_targetCycles);

    // 启动策略：
    // 如果当前位置更靠近 min 侧，先去 max；否则先去 min（这里简单逻辑：离谁远去谁）
    const double distToMin = qAbs(m_position - m_minPos);
    const double distToMax = qAbs(m_position - m_maxPos);

    LOG_INFO << "当前位置: " << m_position << " mm";
    LOG_INFO << "到最小位置距离: " << distToMin << " mm, 到最大位置距离: " << distToMax << " mm";

    m_watchdog.start();
    LOG_INFO << "看门狗定时器已启动";

    if (distToMax < distToMin) {
        LOG_INFO << "决策: 先向最小位置移动";
        startMovingToMin();
    } else {
        LOG_INFO << "决策: 先向最大位置移动";
        startMovingToMax();
    }

    emit message(QString("自动扫描已启动：[最小=%1mm, 最大=%2mm], 速度=%3mm/s, 周期=%4")
                 .arg(m_minPos).arg(m_maxPos).arg(m_speed).arg(m_targetCycles));
}

void TaskManager::startTaskSequence(const QList<TaskStep> &steps, int cycles)
{
    LOG_INFO << "========== 启动任务序列 ==========";
    LOG_INFO << "步骤数: " << steps.size() << ", 周期数: " << cycles;
    
    if (m_state != State::Idle && m_state != State::Fault) {
        LOG_WARN << "任务启动失败: 当前状态不是Idle或Fault";
        emit message("任务正在运行，请先停止");
        return;
    }
    if (steps.isEmpty()) {
        LOG_ERR << "任务启动失败: 步骤列表为空";
        emit fault("任务序列为空");
        return;
    }

    // 打印所有步骤信息
    for (int i = 0; i < steps.size(); ++i) {
        const TaskStep &step = steps.at(i);
        LOG_INFO << "步骤 " << i << ": 类型=" << (int)step.type 
                 << ", 参数1=" << step.param1 << ", 参数2=" << step.param2
                 << ", 描述=" << step.description;
    }

    m_sequenceSteps = steps;
    m_targetCycles = cycles;
    m_completedCycles = 0;
    m_currentStepIndex = -1; // 将在 executeNextStep 中自增为 0
    m_isStepWaiting = false;

    emit progressChanged(m_completedCycles, m_targetCycles);
    
    // 开始执行
    setState(State::StepExecution);
    m_watchdog.start();
    LOG_INFO << "看门狗定时器已启动";
    
    emit message(QString("高级任务序列已启动：步骤数=%1, 周期=%2").arg(steps.size()).arg(cycles));
    
    executeNextStep();
}

/**
 * @brief 暂停任务
 */
void TaskManager::pause()
{
    LOG_INFO << "========== 暂停任务 ==========";
    LOG_INFO << "当前状态: " << (int)m_state;
    
    if (m_state != State::AutoForward && m_state != State::AutoBackward && m_state != State::StepExecution) {
        LOG_WARN << "暂停失败: 当前状态不支持暂停操作";
        return;
    }
    m_lastMotionState = m_state; // 记住暂停前的状态，以便 resume 恢复
    LOG_INFO << "记录暂停前状态: " << (int)m_lastMotionState;
    setState(State::Paused);
    emit requestStop();
    emit message("任务已暂停。");
}

/**
 * @brief 恢复任务
 */
void TaskManager::resume()
{
    LOG_INFO << "========== 恢复任务 ==========";
    LOG_INFO << "当前状态: " << (int)m_state << ", 暂停前状态: " << (int)m_lastMotionState;
    
    if (m_state != State::Paused) {
        LOG_WARN << "恢复失败: 当前状态不是Paused";
        return;
    }
    
    if (m_lastMotionState == State::StepExecution) {
         LOG_INFO << "恢复序列任务执行";
         setState(State::StepExecution);
         // 恢复时如果是等待状态，需要特殊处理，这里简化为继续执行当前步骤
         if (m_isStepWaiting) {
             LOG_INFO << "当前步骤正在等待中";
             // 继续等待
         } else {
             // 重新触发当前步骤的动作（例如继续移动）
             if (m_currentStepIndex >= 0 && m_currentStepIndex < m_sequenceSteps.size()) {
                 LOG_INFO << "重新执行当前步骤: " << m_currentStepIndex;
                 executeStep(m_sequenceSteps.at(m_currentStepIndex));
             }
         }
    } else if (m_lastMotionState == State::AutoForward) {
        LOG_INFO << "恢复向前移动";
        startMovingToMax();
    } else if (m_lastMotionState == State::AutoBackward) {
        LOG_INFO << "恢复向后移动";
        startMovingToMin();
    } else {
        // 兜底：默认去max
        LOG_INFO << "未知的暂停前状态，默认向最大位置移动";
        startMovingToMax();
    }
    emit message("任务已恢复。");
}

/**
 * @brief 停止所有任务
 */
void TaskManager::stopAll()
{
    LOG_INFO << "========== 停止所有任务 ==========";
    LOG_INFO << "当前状态: " << (int)m_state;
    
    if (m_state == State::Idle) {
        LOG_INFO << "当前已是Idle状态，无需停止";
        return;
    }
    setState(State::Stopping);
    emit requestStop();

    // 立即回到 Idle（如果需要等设备确认停稳，可把这个延后到 status 回调中处理）
    setState(State::Idle);
    m_watchdog.stop();
    LOG_INFO << "看门狗定时器已停止";

    emit message("任务已停止。");
}

/**
 * @brief 重置任务
 * 只能在暂停状态下调用，重置任务状态并回到初始位置
 */
void TaskManager::resetTask()
{
    if (m_state != State::Paused) {
        emit message("只能在暂停状态下重置任务");
        return;
    }
    
    // 进入重置状态
    setState(State::Resetting);
    
    // 重置进度
    m_completedCycles = 0;
    emit progressChanged(m_completedCycles, m_targetCycles);
    
    // 智能回到初始位置
    // 对于往返扫描任务，回到最小位置
    // 对于脚本序列任务，回到0位置
    if (!m_sequenceSteps.isEmpty()) {
        // 脚本序列：回到0位置
        m_resetTargetPos = 0.0;
    } else {
        // 往返扫描：回到最小位置
        m_resetTargetPos = m_minPos;
    }
    
    // 检查是否已经在目标位置
    if (reached(m_position, m_resetTargetPos)) {
        // 已经在目标位置，直接完成重置
        setState(State::Idle);
        m_watchdog.stop();
        emit message("任务已重置完成。");
        return;
    }
    
    // 启动看门狗
    m_watchdog.start();
    m_motionStartMs = nowMs();
    
    // 根据当前位置决定移动方向
    if (m_position > m_resetTargetPos) {
        emit requestMoveBackward(20.0); // 使用较慢的速度
        emit message(QString("任务重置中，正在回到初始位置 %1mm...").arg(m_resetTargetPos));
    } else {
        emit requestMoveForward(20.0);
        emit message(QString("任务重置中，正在回到初始位置 %1mm...").arg(m_resetTargetPos));
    }
}

/**
 * @brief 核心逻辑：位置更新回调
 * 
 * 每次收到位置更新时，检查是否到达目标边界。
 * 如果到达边界，触发状态跳转（反向运动或完成任务）。
 */
void TaskManager::onPositionUpdated(double position)
{
    // 只在位置有明显变化时才记录日志，避免日志泛滥
    static double lastLoggedPos = -999.0;
    if (qAbs(position - lastLoggedPos) > 1.0) { // 每移动1mm记录一次
        LOG_INFO << "位置更新: " << position << " mm (状态: " << (int)m_state << ")";
        lastLoggedPos = position;
    }
    
    m_position = position;

    // 只有运行状态下才做边界判断
    if (m_state == State::StepExecution) {
        checkStepCompletion(m_position);
    } else if (m_state == State::AutoForward) {
        if (reached(m_position, m_maxPos)) {
            LOG_INFO << "已到达最大位置: " << m_maxPos << " mm";
            // 到达 max：开始向 min
            startMovingToMin();
        }
    } else if (m_state == State::AutoBackward) {
        if (reached(m_position, m_minPos)) {
            LOG_INFO << "已到达最小位置: " << m_minPos << " mm";
            // 到达 min：完成一次往返
            m_completedCycles++;
            LOG_INFO << "完成周期: " << m_completedCycles << " / " << m_targetCycles;
            emit progressChanged(m_completedCycles, m_targetCycles);

            // 检查是否完成所有周期
            if (m_targetCycles > 0 && m_completedCycles >= m_targetCycles) {
                LOG_INFO << "所有周期已完成，停止任务";
                emit requestStop();
                setState(State::Idle);
                m_watchdog.stop();
                emit message("自动扫描已完成。");
                emit taskCompleted(); // 通知任务完成
                return;
            }

            // 继续下一次：向 max
            LOG_INFO << "开始下一周期";
            startMovingToMax();
        }
    } else if (m_state == State::Resetting) {
        // 检查是否到达重置目标位置
        if (reached(m_position, m_resetTargetPos)) {
            LOG_INFO << "已到达重置目标位置: " << m_resetTargetPos << " mm";
            emit requestStop();
            setState(State::Idle);
            m_watchdog.stop();
            emit message("任务重置完成。");
        }
    }
}

void TaskManager::updateFeedback(const MotionFeedback &fb)
{
    // 如果设备报错，自动任务也应立即中止
    if (fb.errorCode != 0 || fb.status == DeviceStatus::Error) {
        if (m_state != State::Idle && m_state != State::Fault) {
            enterFault(QString("Device Error Code: %1").arg(fb.errorCode));
        }
        return;
    }
    
    // 更新位置，驱动状态机
    onPositionUpdated(fb.position_mm);
}

/**
 * @brief 看门狗超时检测
 */
void TaskManager::onWatchdogTick()
{
    // 序列任务中的 Wait 检查
    if (m_state == State::StepExecution && m_isStepWaiting) {
        qint64 elapsed = nowMs() - m_waitStartTime;
        if (elapsed >= m_waitDurationMs) {
            // 等待结束
            m_isStepWaiting = false;
            executeNextStep();
        }
        return;
    }

    if (m_state != State::AutoForward && m_state != State::AutoBackward && 
        m_state != State::StepExecution && m_state != State::Resetting) {
        return;
    }
    // 如果还没记录开始时间，现在记录
    if (m_motionStartMs <= 0) {
        m_motionStartMs = nowMs();
        return;
    }

    // 序列任务中，如果是在移动，也需要检查超时 (Motion MoveTo)
    // 这里简单共用 edgeTimeout
    const qint64 elapsed = nowMs() - m_motionStartMs;
    if (elapsed > m_edgeTimeoutMs) {
        // 超时未到达目标边界
        QString target = "target";
        if (m_state == State::AutoForward) target = "max";
        else if (m_state == State::AutoBackward) target = "min";
        else if (m_state == State::StepExecution) target = QString("Step %1 Target").arg(m_currentStepIndex);
        else if (m_state == State::Resetting) target = QString("Reset Target %1mm").arg(m_resetTargetPos);

        enterFault(QString("运动超时：向%1移动已超过%2ms，当前位置=%3mm")
                       .arg(target)
                       .arg(elapsed)
                       .arg(m_position));
    }
}

void TaskManager::setState(State s)
{
    if (m_state == s) return;
    LOG_INFO << "状态变更: " << (int)m_state << " -> " << (int)s;
    m_state = s;
    emit stateChanged(m_state);
}

/**
 * @brief 进入故障状态
 */
void TaskManager::enterFault(const QString& reason)
{
    LOG_ERR << "========== 进入故障状态 ==========";
    LOG_ERR << "故障原因: " << reason;
    setState(State::Fault);
    m_watchdog.stop();
    LOG_INFO << "看门狗定时器已停止";
    emit requestStop(); // 尝试停止硬件
    emit fault(reason);
    emit message(QString("FAULT: %1").arg(reason));
    emit taskFailed(reason); // 通知任务失败
}

/**
 * @brief 辅助函数：开始向 Max 方向移动
 */
void TaskManager::startMovingToMax()
{
    LOG_INFO << "---------- 开始向最大位置移动 ----------";
    LOG_INFO << "目标位置: " << m_maxPos << " mm, 速度: " << m_speed << " mm/s";
    setState(State::AutoForward);
    m_motionStartMs = nowMs(); // 重置超时计时
    LOG_INFO << "运动开始时间戳: " << m_motionStartMs;
    emit requestMoveForward(m_speed);
}

/**
 * @brief 辅助函数：开始向 Min 方向移动
 */
void TaskManager::startMovingToMin()
{
    LOG_INFO << "---------- 开始向最小位置移动 ----------";
    LOG_INFO << "目标位置: " << m_minPos << " mm, 速度: " << m_speed << " mm/s";
    setState(State::AutoBackward);
    m_motionStartMs = nowMs(); // 重置超时计时
    LOG_INFO << "运动开始时间戳: " << m_motionStartMs;
    emit requestMoveBackward(m_speed);
}

/**
 * @brief 判断是否到达目标位置
 * 
 * 简单地使用绝对值差与容差比较。
 */
bool TaskManager::reached(double pos, double target) const
{
    return qAbs(pos - target) <= m_tol;
}

// --- 序列执行相关 ---

void TaskManager::executeNextStep()
{
    m_currentStepIndex++;
    if (m_currentStepIndex >= m_sequenceSteps.size()) {
        // 当前周期完成
        m_completedCycles++;
        emit progressChanged(m_completedCycles, m_targetCycles);

        if (m_targetCycles > 0 && m_completedCycles >= m_targetCycles) {
            // 所有周期完成
            emit requestStop();
            setState(State::Idle);
            m_watchdog.stop();
            emit message("高级任务序列已完成。");
            emit taskCompleted(); // 通知任务完成
            return;
        }

        // 下一个周期
        m_currentStepIndex = 0;
    }

    const TaskStep &step = m_sequenceSteps.at(m_currentStepIndex);
    executeStep(step);
}

void TaskManager::executeStep(const TaskStep &step)
{
    QString stepDesc = step.description.isEmpty() ? QString("Step %1").arg(m_currentStepIndex) : step.description;
    emit message(QString("执行步骤: %1").arg(stepDesc));

    switch (step.type) {
    case StepType::MoveTo: {
        double target = step.param1;
        double speed = step.param2 > 0 ? step.param2 : 20.0; // 默认速度 20.0，防止过慢
        
        // 检查目标位置是否超出限位
        double maxPos = ConfigManager::instance().maxPosition();
        if (target > maxPos) {
            enterFault(QString("步骤 %1: 目标位置 %2mm 超过右限位 %3mm").arg(m_currentStepIndex).arg(target).arg(maxPos));
            return;
        }
        if (target < 0.0) {
            enterFault(QString("步骤 %1: 目标位置 %2mm 超过左限位 0mm").arg(m_currentStepIndex).arg(target));
            return;
        }
        
        m_currentStepTargetPos = target;
        m_motionStartMs = nowMs(); // 重置超时
        
        // 预判：如果已经到位，直接进入下一步，避免原地抖动
        if (reached(m_position, target)) {
             emit message(QString("步骤 %1: 已在目标位置 %2，跳过移动").arg(m_currentStepIndex).arg(target));
             // 使用 QTimer::singleShot 异步调用下一 步，避免递归过深
             QTimer::singleShot(0, this, [this](){ executeNextStep(); });
             break;
        }

        // 简单判断方向
        if (target > m_position) {
             emit message(QString("步骤 %1: 向前移动到 %2mm，速度 %3%").arg(m_currentStepIndex).arg(target).arg(speed));
             emit requestMoveForward(speed);
        } else {
             emit message(QString("步骤 %1: 向后移动到 %2mm，速度 %3%").arg(m_currentStepIndex).arg(target).arg(speed));
             emit requestMoveBackward(speed);
        }
        break;
    }
    case StepType::Wait: {
        int ms = static_cast<int>(step.param1);
        m_waitDurationMs = ms;
        m_waitStartTime = nowMs();
        m_isStepWaiting = true;
        emit message(QString("步骤 %1: 等待 %2ms").arg(m_currentStepIndex).arg(ms));
        emit requestStop(); // 等待时停止运动
        break;
    }
    case StepType::SetSpeed: {
        double newSpeed = step.param1;
        if (newSpeed > 0) {
            m_speed = newSpeed;
            emit message(QString("步骤 %1: 设置速度为 %2%").arg(m_currentStepIndex).arg(newSpeed));
        }
        executeNextStep(); // 立即执行下一步
        break;
    }
    }
}

void TaskManager::checkStepCompletion(double currentPos)
{
    if (m_state != State::StepExecution) return;
    if (m_isStepWaiting) return; // 等待中由 Watchdog 处理

    // 如果是 MoveTo 步骤
    if (m_currentStepIndex >= 0 && m_currentStepIndex < m_sequenceSteps.size()) {
        const TaskStep &step = m_sequenceSteps.at(m_currentStepIndex);
        if (step.type == StepType::MoveTo) {
             if (reached(currentPos, m_currentStepTargetPos)) {
                 executeNextStep();
             }
        }
    }
}
