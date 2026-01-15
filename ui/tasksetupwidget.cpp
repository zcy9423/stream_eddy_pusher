#include "tasksetupwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QDebug>

TaskSetupWidget::TaskSetupWidget(QWidget *parent)
    : QGroupBox("任务配置管理", parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 40, 30, 30);

    // 1. 输入表单
    QHBoxLayout *inputLayout = new QHBoxLayout();
    
    m_editOperator = new QLineEdit(this);
    m_editOperator->setPlaceholderText("操作员姓名");
    m_editOperator->setMinimumHeight(35);

    m_editTubeId = new QLineEdit(this);
    m_editTubeId->setPlaceholderText("传热管编号");
    m_editTubeId->setMinimumHeight(35);
    
    m_btnCreate = new QPushButton("创建任务", this);
    m_btnCreate->setObjectName("btnAction");
    m_btnCreate->setMinimumHeight(35);
    m_btnCreate->setCursor(Qt::PointingHandCursor);
    m_btnCreate->setEnabled(false);

    inputLayout->addWidget(new QLabel("操作员:"));
    inputLayout->addWidget(m_editOperator);
    inputLayout->addWidget(new QLabel("传热管编号:"));
    inputLayout->addWidget(m_editTubeId);
    inputLayout->addWidget(m_btnCreate);

    mainLayout->addLayout(inputLayout);

    // 2. 任务列表
    m_taskTable = new QTableWidget(this);
    m_taskTable->setColumnCount(5);
    m_taskTable->setHorizontalHeaderLabels({"任务ID", "创建时间", "操作员", "管道编号", "操作"});
    m_taskTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_taskTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_taskTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    mainLayout->addWidget(m_taskTable);

    // 信号连接
    connect(m_editOperator, &QLineEdit::textChanged, this, &TaskSetupWidget::checkInput);
    connect(m_editTubeId, &QLineEdit::textChanged, this, &TaskSetupWidget::checkInput);
    
    connect(m_btnCreate, &QPushButton::clicked, this, [this](){
        emit createTaskClicked(m_editOperator->text().trimmed(), m_editTubeId->text().trimmed());
        m_editTubeId->clear(); // 清空编号方便下一次输入，操作员通常不变
        checkInput();
    });
}

QString TaskSetupWidget::operatorName() const
{
    return m_editOperator->text().trimmed();
}

QString TaskSetupWidget::tubeId() const
{
    return m_editTubeId->text().trimmed();
}

void TaskSetupWidget::updateTaskState(int taskId, const QString& opName, const QString& tubeId)
{
    m_activeTaskId = taskId;
    
    // 如果有新任务ID且列表中没有，则添加一行
    if (taskId != -1) {
        // 检查列表中是否已存在
        bool exists = false;
        for (int i = 0; i < m_taskTable->rowCount(); ++i) {
            if (m_taskTable->item(i, 0)->text().toInt() == taskId) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            int row = m_taskTable->rowCount();
            m_taskTable->insertRow(row);
            
            m_taskTable->setItem(row, 0, new QTableWidgetItem(QString::number(taskId)));
            m_taskTable->setItem(row, 1, new QTableWidgetItem(QDateTime::currentDateTime().toString("HH:mm:ss")));
            // 使用传入的参数填充，或者用"-"占位（如果未提供）
            m_taskTable->setItem(row, 2, new QTableWidgetItem(opName.isEmpty() ? "-" : opName)); 
            m_taskTable->setItem(row, 3, new QTableWidgetItem(tubeId.isEmpty() ? "-" : tubeId));
            
            // 操作按钮
            QWidget *btnWidget = new QWidget();
            QHBoxLayout *layout = new QHBoxLayout(btnWidget);
            layout->setContentsMargins(5, 2, 5, 2);
            layout->setSpacing(10);
            
            QPushButton *btnStart = new QPushButton("开始");
            QPushButton *btnEnd = new QPushButton("结束");
            
            // 存入 ID
            btnStart->setProperty("taskId", taskId);
            btnEnd->setProperty("taskId", taskId);
            
            connect(btnStart, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
            connect(btnEnd, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
            
            layout->addWidget(btnStart);
            layout->addWidget(btnEnd);
            m_taskTable->setCellWidget(row, 4, btnWidget);
        }
    }
    
    // 刷新所有按钮状态
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QWidget *w = m_taskTable->cellWidget(i, 4);
        if (!w) continue;
        
        QPushButton *btnStart = w->findChild<QPushButton*>(); // 第一个是开始
        QPushButton *btnEnd = w->findChildren<QPushButton*>().last(); // 第二个是结束
        
        int rowTaskId = m_taskTable->item(i, 0)->text().toInt();
        
        if (m_activeTaskId == -1) {
            // 没有活跃任务 -> 理论上所有未完成的任务都可开始（需配合数据库状态，这里简化处理）
            // 如果是刚刚结束的任务，这里其实应该禁用开始，防止重复开始同一ID
            // 但因为目前 taskId 是递增且唯一的，旧 ID 不会重用，所以禁用所有旧任务的“开始”比较安全
            // 除非有“恢复任务”功能。这里假设：已结束的任务不能再开始。
            btnStart->setEnabled(false); 
            btnEnd->setEnabled(false);
        } else {
            if (rowTaskId == m_activeTaskId) {
                // 是当前活跃任务
                // 需求变更：创建任务后不默认执行，所以这里允许点击“开始”
                // 如果是“正在运行中”（即已经点击了开始），则禁用开始按钮
                
                // 但这里 updateTaskState 通常是在任务创建后调用的
                // 我们如何区分“新建但未开始”和“正在进行中”？
                // 暂时利用按钮文本来判断，或者增加状态参数
                
                // 简单逻辑：如果是活跃任务，总是启用“开始”，点击后由 Controller 决定是否跳转
                // 为了避免混淆，如果已经在进行中，Controller 应该不需要重新 create
                // 这里我们假设：只要是 Active Task，就是已经 Created 的。
                // 此时“开始”按钮的作用是“进入控制模式”
                
                btnStart->setEnabled(true);
                btnStart->setText("开始"); 
                btnEnd->setEnabled(true);
            } else {
                // 其他任务（旧任务） -> 禁用
                btnStart->setEnabled(false);
                btnEnd->setEnabled(false);
            }
        }
    }
}

void TaskSetupWidget::checkInput()
{
    bool valid = !m_editOperator->text().trimmed().isEmpty() &&
                 !m_editTubeId->text().trimmed().isEmpty();
    m_btnCreate->setEnabled(valid);
}

void TaskSetupWidget::onTableBtnClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    int taskId = btn->property("taskId").toInt();
    QString text = btn->text();
    
    if (text == "开始") {
        emit startTaskClicked(taskId);
    } else if (text == "结束") {
        emit endTaskClicked();
    }
}
