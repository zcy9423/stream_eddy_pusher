#include "autotaskwidget.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>

AutoTaskWidget::AutoTaskWidget(QWidget *parent) : QGroupBox("自动往返扫描", parent)
{
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 20));
    shadow->setOffset(0, 2);
    this->setGraphicsEffect(shadow);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QVBoxLayout *autoLayout = new QVBoxLayout(this);
    autoLayout->setContentsMargins(30, 40, 30, 30);
    autoLayout->setSpacing(20);

    QGridLayout *paramGrid = new QGridLayout();
    paramGrid->setHorizontalSpacing(20);
    paramGrid->setVerticalSpacing(15);

    auto addParam = [&](QString label, QWidget* widget, int row, int col) {
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("color: #7f8c8d; font-weight: bold;");
        paramGrid->addWidget(lbl, row, col);
        paramGrid->addWidget(widget, row, col + 1);
    };

    m_spinMinPos = new QDoubleSpinBox(); m_spinMinPos->setRange(0, 1000); m_spinMinPos->setSuffix(" mm");
    m_spinMaxPos = new QDoubleSpinBox(); m_spinMaxPos->setRange(0, 1000); m_spinMaxPos->setSuffix(" mm"); m_spinMaxPos->setValue(100.0);
    m_spinAutoSpeed = new QDoubleSpinBox(); m_spinAutoSpeed->setRange(0, 100); m_spinAutoSpeed->setSuffix(" %"); m_spinAutoSpeed->setValue(20.0);
    m_spinCycles = new QSpinBox(); m_spinCycles->setRange(0, 9999); m_spinCycles->setSuffix(" 次"); m_spinCycles->setValue(5);

    addParam("起点 (Min):", m_spinMinPos, 0, 0);
    addParam("终点 (Max):", m_spinMaxPos, 0, 2);
    addParam("速度 (Speed):", m_spinAutoSpeed, 1, 0);
    addParam("周期 (Cycles):", m_spinCycles, 1, 2);

    autoLayout->addLayout(paramGrid);

    QHBoxLayout *progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(15);
    
    m_lblCycleCount = new QLabel("0 / 5");
    m_lblCycleCount->setStyleSheet("font-weight: bold; color: #666;");

    progressLayout->addWidget(new QLabel("任务进度:"));
    progressLayout->addWidget(m_progressBar);
    progressLayout->addWidget(m_lblCycleCount);

    autoLayout->addLayout(progressLayout);

    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_btnStartTask = new QPushButton("开始扫描任务");
    m_btnStartTask->setObjectName("btnAction");
    m_btnStartTask->setCursor(Qt::PointingHandCursor);
    m_btnStartTask->setMinimumHeight(45);

    m_btnPauseTask = new QPushButton("暂停任务");
    m_btnPauseTask->setCursor(Qt::PointingHandCursor);
    m_btnPauseTask->setMinimumHeight(45);
    m_btnPauseTask->setEnabled(false); // 初始不可用

    actionLayout->addWidget(m_btnStartTask);
    actionLayout->addWidget(m_btnPauseTask);
    autoLayout->addLayout(actionLayout);

    connect(m_btnStartTask, &QPushButton::clicked, this, [this](){
        emit startTaskClicked(m_spinMinPos->value(), 
                            m_spinMaxPos->value(), 
                            m_spinAutoSpeed->value(), 
                            m_spinCycles->value());
    });

    connect(m_btnPauseTask, &QPushButton::clicked, this, [this](){
        if (m_isPaused) {
            emit resumeTaskClicked();
        } else {
            emit pauseTaskClicked();
        }
    });

    // 默认禁用，等待连接成功且任务创建后启用
    this->setEnabled(false);
}

void AutoTaskWidget::updateProgress(int completed, int total)
{
    m_lblCycleCount->setText(QString("%1 / %2").arg(completed).arg(total));
    if (total > 0) {
        int pct = (completed * 100) / total;
        m_progressBar->setValue(pct);
    }
}

void AutoTaskWidget::updateState(TaskManager::State state)
{
    switch (state) {
    case TaskManager::State::Idle:
    case TaskManager::State::Fault:
    case TaskManager::State::Stopping:
        m_btnStartTask->setEnabled(true);
        m_btnStartTask->setText("开始扫描任务");
        m_btnPauseTask->setEnabled(false);
        m_btnPauseTask->setText("暂停任务");
        m_isPaused = false;
        m_progressBar->setValue(0);
        break;
    case TaskManager::State::AutoForward:
    case TaskManager::State::AutoBackward:
        m_btnStartTask->setEnabled(false);
        m_btnStartTask->setText("任务运行中...");
        m_btnPauseTask->setEnabled(true);
        m_btnPauseTask->setText("暂停任务");
        m_isPaused = false;
        break;
    case TaskManager::State::Paused:
        m_btnPauseTask->setText("继续任务");
        m_isPaused = true;
        break;
    }
}
