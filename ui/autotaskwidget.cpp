#include "autotaskwidget.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QHeaderView>
#include <QMessageBox>

AutoTaskWidget::AutoTaskWidget(QWidget *parent) : QGroupBox("自动任务控制", parent)
{
    // 移除阴影
    // QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect; ...
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 30, 10, 10);

    // 创建选项卡
    QTabWidget *tabWidget = new QTabWidget();
    // 移除硬编码样式，使用全局 QSS
    
    QWidget *tabSimple = new QWidget();
    QWidget *tabAdvanced = new QWidget();
    
    setupSimpleTab(tabSimple);
    setupAdvancedTab(tabAdvanced);
    
    tabWidget->addTab(tabSimple, "标准往返扫描");
    tabWidget->addTab(tabAdvanced, "高级脚本序列");
    
    mainLayout->addWidget(tabWidget);

    // 公共进度条区域
    QHBoxLayout *progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(15);
    
    m_lblCycleCount = new QLabel("0 / 5");
    m_lblCycleCount->setStyleSheet("font-weight: bold; color: #7F8C8D;"); // 适配亮色

    progressLayout->addWidget(new QLabel("任务进度:"));
    progressLayout->addWidget(m_progressBar);
    progressLayout->addWidget(m_lblCycleCount);

    mainLayout->addLayout(progressLayout);
    
    // 公共控制按钮 (暂停/恢复/重置)
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    // 注意：开始按钮分别在各自的 Tab 中，但暂停和重置是全局的
    m_btnPauseTask = new QPushButton("暂停任务");
    m_btnPauseTask->setCursor(Qt::PointingHandCursor);
    m_btnPauseTask->setMinimumHeight(40);
    m_btnPauseTask->setEnabled(false);
    
    m_btnResetTask = new QPushButton("重置任务");
    m_btnResetTask->setCursor(Qt::PointingHandCursor);
    m_btnResetTask->setMinimumHeight(40);
    m_btnResetTask->setEnabled(false);
    m_btnResetTask->setStyleSheet("QPushButton { background-color: #F39C12; color: white; border: none; } QPushButton:hover { background-color: #E67E22; } QPushButton:disabled { background-color: #F7DC6F; }");
    
    connect(m_btnPauseTask, &QPushButton::clicked, this, [this](){
        if (m_isPaused) {
            emit resumeTaskClicked();
        } else {
            emit pauseTaskClicked();
        }
    });
    
    connect(m_btnResetTask, &QPushButton::clicked, this, [this](){
        emit resetTaskClicked();
    });

    controlLayout->addWidget(m_btnPauseTask);
    controlLayout->addWidget(m_btnResetTask);
    mainLayout->addLayout(controlLayout);

    // 默认禁用，等待连接成功且任务创建后启用
    this->setEnabled(false);
}

void AutoTaskWidget::setupSimpleTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(15);
    
    QGridLayout *paramGrid = new QGridLayout();
    paramGrid->setVerticalSpacing(16);
    paramGrid->setHorizontalSpacing(4); // 标签和输入框之间的紧凑间距

    auto configLabel = [](QLabel *lbl) {
        lbl->setStyleSheet("color: #7F8C8D; font-weight: bold;");
        lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    };

    auto configSpin = [](QAbstractSpinBox *widget) {
        widget->setFixedWidth(120); 
        widget->setMinimumHeight(30);
        widget->setButtonSymbols(QAbstractSpinBox::NoButtons);
    };

    m_spinMinPos = new QDoubleSpinBox(); m_spinMinPos->setRange(0, 1000); m_spinMinPos->setSuffix(" mm"); configSpin(m_spinMinPos);
    m_spinMaxPos = new QDoubleSpinBox(); m_spinMaxPos->setRange(0, 1000); m_spinMaxPos->setSuffix(" mm"); m_spinMaxPos->setValue(100.0); configSpin(m_spinMaxPos);
    m_spinAutoSpeed = new QDoubleSpinBox(); m_spinAutoSpeed->setRange(0, 100); m_spinAutoSpeed->setSuffix(" %"); m_spinAutoSpeed->setValue(20.0); configSpin(m_spinAutoSpeed);
    m_spinCycles = new QSpinBox(); m_spinCycles->setRange(0, 9999); m_spinCycles->setValue(5); configSpin(m_spinCycles);

    // Row 0
    QLabel *lblMin = new QLabel("起点 (Min):"); configLabel(lblMin);
    paramGrid->addWidget(lblMin, 0, 0);
    paramGrid->addWidget(m_spinMinPos, 0, 1);
    
    // Spacer Col 2 (Group Spacing)
    paramGrid->addItem(new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum), 0, 2);

    QLabel *lblMax = new QLabel("终点 (Max):"); configLabel(lblMax);
    paramGrid->addWidget(lblMax, 0, 3);
    paramGrid->addWidget(m_spinMaxPos, 0, 4);

    // Row 1
    QLabel *lblSpeed = new QLabel("速度 (Speed):"); configLabel(lblSpeed);
    paramGrid->addWidget(lblSpeed, 1, 0);
    paramGrid->addWidget(m_spinAutoSpeed, 1, 1);
    
    QLabel *lblCycles = new QLabel("周期 (Cycles):"); configLabel(lblCycles);
    paramGrid->addWidget(lblCycles, 1, 3);
    paramGrid->addWidget(m_spinCycles, 1, 4);

    // Push everything to left
    paramGrid->setColumnStretch(5, 1);

    layout->addLayout(paramGrid);

    m_btnStartTask = new QPushButton("开始扫描任务");
    m_btnStartTask->setObjectName("btnAction");
    m_btnStartTask->setCursor(Qt::PointingHandCursor);
    m_btnStartTask->setMinimumHeight(40);
    
    connect(m_btnStartTask, &QPushButton::clicked, this, [this](){
        emit startTaskClicked(m_spinMinPos->value(), 
                            m_spinMaxPos->value(), 
                            m_spinAutoSpeed->value(), 
                            m_spinCycles->value());
    });
    
    layout->addWidget(m_btnStartTask);
}

void AutoTaskWidget::setupAdvancedTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);
    
    // 1. 步骤列表
    m_tableSteps = new QTableWidget();
    m_tableSteps->setColumnCount(3);
    m_tableSteps->setHorizontalHeaderLabels({"类型", "参数1", "参数2"});
    m_tableSteps->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableSteps->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableSteps->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_tableSteps);
    
    // 2. 添加步骤区域
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(12);
    
    m_comboStepType = new QComboBox();
    m_comboStepType->addItem("移动到 (MoveTo)", static_cast<int>(TaskManager::StepType::MoveTo));
    m_comboStepType->addItem("等待 (Wait)", static_cast<int>(TaskManager::StepType::Wait));
    // m_comboStepType->addItem("设速度 (SetSpeed)", static_cast<int>(TaskManager::StepType::SetSpeed)); // 移除设速度
    
    m_spinStepParam1 = new QDoubleSpinBox(); m_spinStepParam1->setRange(0, 99999); m_spinStepParam1->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_spinStepParam2 = new QDoubleSpinBox(); m_spinStepParam2->setRange(0, 99999); m_spinStepParam2->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_spinStepParam2->setValue(20.0); // 默认速度
    
    QPushButton *btnAdd = new QPushButton("添加步骤");
    connect(btnAdd, &QPushButton::clicked, this, &AutoTaskWidget::onAddStep);
    
    auto createInlineField = [&](const QString &label, QWidget *widget) -> QLabel* {
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("color: #7F8C8D; font-weight: bold;");
        lbl->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        
        // 设置输入框/下拉框的固定宽度
        if (qobject_cast<QComboBox*>(widget)) {
            widget->setFixedWidth(140);
        } else {
            widget->setFixedWidth(100);
        }
        widget->setMinimumHeight(30);

        inputLayout->addWidget(lbl);
        inputLayout->addSpacing(4);
        inputLayout->addWidget(widget);
        inputLayout->addSpacing(20);
        
        return lbl;
    };

    createInlineField("类型:", m_comboStepType);
    QLabel *lblP1 = createInlineField("P1:", m_spinStepParam1);
    QLabel *lblP2 = createInlineField("P2:", m_spinStepParam2);
    inputLayout->addWidget(btnAdd);
    inputLayout->addSpacing(20);

    // 新增：循环次数
    m_spinSeqCycles = new QSpinBox();
    m_spinSeqCycles->setRange(1, 9999);
    m_spinSeqCycles->setValue(1);
    m_spinSeqCycles->setButtonSymbols(QAbstractSpinBox::NoButtons);
    createInlineField("循环次数:", m_spinSeqCycles);

    inputLayout->addStretch(); // 确保左对齐
    
    layout->addLayout(inputLayout);
    
    // 3. 编辑按钮
    QHBoxLayout *editLayout = new QHBoxLayout();
    QPushButton *btnRemove = new QPushButton("删除选中");
    QPushButton *btnClear = new QPushButton("清空所有");
    
    connect(btnRemove, &QPushButton::clicked, this, &AutoTaskWidget::onRemoveStep);
    connect(btnClear, &QPushButton::clicked, this, &AutoTaskWidget::onClearSteps);
    
    editLayout->addWidget(btnRemove);
    editLayout->addWidget(btnClear);
    editLayout->addStretch();
    
    // 移除底部的循环设置
    // m_spinSeqCycles = new QSpinBox(); ...
    
    layout->addLayout(editLayout);

    m_btnRunSeq = new QPushButton("执行脚本序列");
    m_btnRunSeq->setCursor(Qt::PointingHandCursor);
    m_btnRunSeq->setMinimumHeight(40);
    connect(m_btnRunSeq, &QPushButton::clicked, this, &AutoTaskWidget::onRunSequence);
    layout->addWidget(m_btnRunSeq);
    
    // 动态提示
    connect(m_comboStepType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, lblP1, lblP2](int index){
        TaskManager::StepType type = static_cast<TaskManager::StepType>(m_comboStepType->currentData().toInt());
        if (type == TaskManager::StepType::MoveTo) {
            lblP1->setText("位置 (mm):");
            m_spinStepParam1->setSuffix(" mm");
            m_spinStepParam1->setToolTip("目标位置");
            
            lblP2->setText("速度 (%):");
            lblP2->setVisible(true);
            m_spinStepParam2->setVisible(true);
            m_spinStepParam2->setSuffix(" %");
            m_spinStepParam2->setEnabled(true);
            m_spinStepParam2->setToolTip("移动速度 (0表示使用默认)");
        } else if (type == TaskManager::StepType::Wait) {
            lblP1->setText("时间 (ms):");
            m_spinStepParam1->setSuffix(" ms");
            m_spinStepParam1->setToolTip("等待时间");
            
            lblP2->setVisible(false);
            m_spinStepParam2->setVisible(false);
        }
    });
    // 触发一次 update
    emit m_comboStepType->currentIndexChanged(0);
}

void AutoTaskWidget::onAddStep()
{
    int row = m_tableSteps->rowCount();
    m_tableSteps->insertRow(row);
    
    TaskManager::StepType type = static_cast<TaskManager::StepType>(m_comboStepType->currentData().toInt());
    QString typeStr = m_comboStepType->currentText();
    
    QTableWidgetItem *itemType = new QTableWidgetItem(typeStr);
    itemType->setData(Qt::UserRole, static_cast<int>(type));
    
    QTableWidgetItem *itemP1 = new QTableWidgetItem(QString::number(m_spinStepParam1->value()));
    QTableWidgetItem *itemP2 = new QTableWidgetItem(QString::number(m_spinStepParam2->value()));
    
    m_tableSteps->setItem(row, 0, itemType);
    m_tableSteps->setItem(row, 1, itemP1);
    m_tableSteps->setItem(row, 2, itemP2);
}

void AutoTaskWidget::onRemoveStep()
{
    int row = m_tableSteps->currentRow();
    if (row >= 0) {
        m_tableSteps->removeRow(row);
    }
}

void AutoTaskWidget::onClearSteps()
{
    m_tableSteps->setRowCount(0);
}

void AutoTaskWidget::onRunSequence()
{
    int count = m_tableSteps->rowCount();
    if (count == 0) {
        QMessageBox::warning(this, "提示", "请先添加至少一个步骤");
        return;
    }
    
    QList<TaskManager::TaskStep> steps;
    for (int i = 0; i < count; ++i) {
        TaskManager::TaskStep step;
        step.type = static_cast<TaskManager::StepType>(m_tableSteps->item(i, 0)->data(Qt::UserRole).toInt());
        step.param1 = m_tableSteps->item(i, 1)->text().toDouble();
        step.param2 = m_tableSteps->item(i, 2)->text().toDouble();
        
        // 自动生成描述
        if (step.type == TaskManager::StepType::MoveTo) {
            step.description = QString("MoveTo %1mm @ %2%").arg(step.param1).arg(step.param2);
        } else if (step.type == TaskManager::StepType::Wait) {
            step.description = QString("Wait %1ms").arg(step.param1);
        } else if (step.type == TaskManager::StepType::SetSpeed) {
            step.description = QString("SetSpeed %1%").arg(step.param1);
        }
        
        steps.append(step);
    }
    
    emit startSequenceClicked(steps, m_spinSeqCycles->value());
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
    bool isRunning = (state != TaskManager::State::Idle && 
                      state != TaskManager::State::Fault && 
                      state != TaskManager::State::Stopping);
                      
    // 控制按钮状态
    if (isRunning) {
        // 运行时：禁用开始按钮
        m_btnStartTask->setEnabled(false);
        m_btnRunSeq->setEnabled(false);
        
        if (state == TaskManager::State::Paused) {
             m_btnPauseTask->setText("继续任务");
             m_btnPauseTask->setEnabled(true);
             m_btnResetTask->setText("重置任务");
             m_btnResetTask->setEnabled(true);  // 只有暂停时才能重置
             m_isPaused = true;
        } else if (state == TaskManager::State::Resetting) {
             // 重置中：禁用所有操作
             m_btnPauseTask->setEnabled(false);
             m_btnResetTask->setText("重置中...");
             m_btnResetTask->setEnabled(false);
             m_isPaused = false;
        } else {
             m_btnPauseTask->setText("暂停任务");
             m_btnPauseTask->setEnabled(true);
             m_btnResetTask->setText("重置任务");
             m_btnResetTask->setEnabled(false); // 运行时不能重置
             m_isPaused = false;
        }
        
        // 运行时禁用编辑
        m_tableSteps->setEnabled(false);
    } else {
        // 空闲时：启用开始，禁用暂停和重置
        m_btnStartTask->setEnabled(true);
        m_btnRunSeq->setEnabled(true);
        m_btnPauseTask->setEnabled(false);
        m_btnResetTask->setEnabled(false);
        m_btnPauseTask->setText("暂停任务");
        m_btnResetTask->setText("重置任务");
        m_isPaused = false;
        m_progressBar->setValue(0);
        
        m_tableSteps->setEnabled(true);
    }
    
    // 更新按钮文字状态
    if (state == TaskManager::State::AutoForward || state == TaskManager::State::AutoBackward) {
        m_btnStartTask->setText("自动扫描运行中...");
    } else if (state == TaskManager::State::StepExecution) {
        m_btnRunSeq->setText("脚本正在执行...");
    } else if (state == TaskManager::State::Resetting) {
        m_btnStartTask->setText("重置中，请稍候...");
        m_btnRunSeq->setText("重置中，请稍候...");
    } else {
        m_btnStartTask->setText("开始扫描任务");
        m_btnRunSeq->setText("执行脚本序列");
    }
}
