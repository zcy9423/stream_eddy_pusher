#include "taskconfigdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

TaskConfigDialog::TaskConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    this->setWindowTitle("配置检测任务");
    this->setFixedSize(350, 200);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    // 操作员输入
    QLabel *lblOp = new QLabel("操作员姓名:", this);
    m_editOperator = new QLineEdit(this);
    m_editOperator->setPlaceholderText("请输入姓名...");
    
    // 管道ID输入
    QLabel *lblTube = new QLabel("传热管编号 (行-列):", this);
    m_editTubeId = new QLineEdit(this);
    m_editTubeId->setPlaceholderText("例如: R01-C05");

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_btnOk = new QPushButton("开始任务", this);
    m_btnOk->setDefault(true);
    m_btnOk->setEnabled(false); // 默认禁用，直到输入有效
    
    QPushButton *btnCancel = new QPushButton("取消", this);
    
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(m_btnOk);

    // 添加到主布局
    mainLayout->addWidget(lblOp);
    mainLayout->addWidget(m_editOperator);
    mainLayout->addWidget(lblTube);
    mainLayout->addWidget(m_editTubeId);
    mainLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // 连接信号
    connect(m_editOperator, &QLineEdit::textChanged, this, &TaskConfigDialog::checkInput);
    connect(m_editTubeId, &QLineEdit::textChanged, this, &TaskConfigDialog::checkInput);
    
    connect(m_btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

QString TaskConfigDialog::operatorName() const
{
    return m_editOperator->text().trimmed();
}

QString TaskConfigDialog::tubeId() const
{
    return m_editTubeId->text().trimmed();
}

void TaskConfigDialog::checkInput()
{
    bool valid = !m_editOperator->text().trimmed().isEmpty() &&
                 !m_editTubeId->text().trimmed().isEmpty();
    m_btnOk->setEnabled(valid);
}
