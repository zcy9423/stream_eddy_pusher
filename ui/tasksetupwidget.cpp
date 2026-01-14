#include "tasksetupwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

TaskSetupWidget::TaskSetupWidget(QWidget *parent)
    : QGroupBox("任务配置管理", parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 40, 30, 30);

    // 1. 输入表单
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setSpacing(15);

    m_editOperator = new QLineEdit(this);
    m_editOperator->setPlaceholderText("请输入操作员姓名");
    m_editOperator->setMinimumHeight(35);

    m_editTubeId = new QLineEdit(this);
    m_editTubeId->setPlaceholderText("请输入传热管编号 (如 R01-C05)");
    m_editTubeId->setMinimumHeight(35);

    formLayout->addRow("操作员:", m_editOperator);
    formLayout->addRow("管道编号:", m_editTubeId);

    mainLayout->addLayout(formLayout);

    // 2. 状态显示
    m_lblCurrentStatus = new QLabel("当前状态: 空闲 (未创建任务)", this);
    m_lblCurrentStatus->setStyleSheet("color: #7f8c8d; font-size: 14px; margin-top: 10px;");
    m_lblCurrentStatus->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_lblCurrentStatus);

    // 3. 操作按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20);

    m_btnStart = new QPushButton("创建并开始任务", this);
    m_btnStart->setObjectName("btnAction"); // 复用样式
    m_btnStart->setMinimumHeight(45);
    m_btnStart->setCursor(Qt::PointingHandCursor);
    m_btnStart->setEnabled(false); // 初始禁用

    m_btnEnd = new QPushButton("结束当前任务", this);
    m_btnEnd->setObjectName("btnStop"); // 复用红色样式
    m_btnEnd->setMinimumHeight(45);
    m_btnEnd->setCursor(Qt::PointingHandCursor);
    m_btnEnd->setEnabled(false); // 初始禁用

    btnLayout->addWidget(m_btnStart);
    btnLayout->addWidget(m_btnEnd);

    mainLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // 信号连接
    connect(m_editOperator, &QLineEdit::textChanged, this, &TaskSetupWidget::checkInput);
    connect(m_editTubeId, &QLineEdit::textChanged, this, &TaskSetupWidget::checkInput);
    
    connect(m_btnStart, &QPushButton::clicked, this, &TaskSetupWidget::startTaskClicked);
    connect(m_btnEnd, &QPushButton::clicked, this, &TaskSetupWidget::endTaskClicked);
}

QString TaskSetupWidget::operatorName() const
{
    return m_editOperator->text().trimmed();
}

QString TaskSetupWidget::tubeId() const
{
    return m_editTubeId->text().trimmed();
}

void TaskSetupWidget::updateTaskState(int taskId)
{
    if (taskId != -1) {
        // 任务进行中
        m_lblCurrentStatus->setText(QString("当前任务 ID: %1 (进行中)").arg(taskId));
        m_lblCurrentStatus->setStyleSheet("color: #27ae60; font-weight: bold; font-size: 16px;");
        
        m_editOperator->setEnabled(false);
        m_editTubeId->setEnabled(false);
        m_btnStart->setEnabled(false);
        m_btnEnd->setEnabled(true);
    } else {
        // 空闲
        m_lblCurrentStatus->setText("当前状态: 空闲 (请创建新任务)");
        m_lblCurrentStatus->setStyleSheet("color: #7f8c8d; font-size: 14px;");
        
        m_editOperator->setEnabled(true);
        m_editTubeId->setEnabled(true);
        m_btnEnd->setEnabled(false);
        checkInput(); // 重新评估 Start 按钮状态
    }
}

void TaskSetupWidget::checkInput()
{
    // 只有在非任务状态下才检查输入
    if (!m_editOperator->isEnabled()) return;

    bool valid = !m_editOperator->text().trimmed().isEmpty() &&
                 !m_editTubeId->text().trimmed().isEmpty();
    m_btnStart->setEnabled(valid);
}
