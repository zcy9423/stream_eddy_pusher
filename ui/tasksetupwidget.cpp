#include "tasksetupwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QDebug>
#include <QSqlTableModel>

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
    m_taskTable->setColumnCount(6);
    m_taskTable->setHorizontalHeaderLabels({"任务ID", "创建时间", "操作员", "管道编号", "当前状态", "操作"});
    m_taskTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_taskTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_taskTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
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

void TaskSetupWidget::loadHistory(QSqlTableModel *model)
{
    if (!model) return;

    model->select();

    m_taskTable->setRowCount(0);

    const int idCol = model->fieldIndex("id");
    const int timeCol = model->fieldIndex("start_time");
    const int opCol = model->fieldIndex("operator_name");
    const int tubeCol = model->fieldIndex("tube_id");

    const int statusCol = model->fieldIndex("status");
    const int rows = model->rowCount();
    for (int r = 0; r < rows; ++r) {
        const int taskId = model->data(model->index(r, idCol)).toInt();
        const QDateTime startTime = model->data(model->index(r, timeCol)).toDateTime();
        const QString opName = model->data(model->index(r, opCol)).toString();
        const QString tube = model->data(model->index(r, tubeCol)).toString();
        QString status = "stop";
        if (statusCol != -1) {
            status = model->data(model->index(r, statusCol)).toString();
        } else if (taskId == m_activeTaskId) {
            status = "create";
        }

        const int row = m_taskTable->rowCount();
        m_taskTable->insertRow(row);

        m_taskTable->setItem(row, 0, new QTableWidgetItem(QString::number(taskId)));
        m_taskTable->setItem(row, 1, new QTableWidgetItem(startTime.isValid() ? startTime.toString("yyyy-MM-dd HH:mm:ss") : "-"));
        m_taskTable->setItem(row, 2, new QTableWidgetItem(opName.isEmpty() ? "-" : opName));
        m_taskTable->setItem(row, 3, new QTableWidgetItem(tube.isEmpty() ? "-" : tube));
        m_taskTable->setItem(row, 4, new QTableWidgetItem(status));

        QWidget *btnWidget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(btnWidget);
        layout->setContentsMargins(5, 2, 5, 2);
        layout->setSpacing(10);

        QPushButton *btnStart = new QPushButton("开始");
        QPushButton *btnEnd = new QPushButton("结束");
        QPushButton *btnDelete = new QPushButton("删除");

        btnStart->setObjectName("btnStart");
        btnEnd->setObjectName("btnEnd");
        btnDelete->setObjectName("btnDelete");

        btnStart->setProperty("taskId", taskId);
        btnEnd->setProperty("taskId", taskId);
        btnDelete->setProperty("taskId", taskId);

        connect(btnStart, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
        connect(btnEnd, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
        connect(btnDelete, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);

        layout->addWidget(btnStart);
        layout->addWidget(btnEnd);
        layout->addWidget(btnDelete);
        m_taskTable->setCellWidget(row, 5, btnWidget);
    }

    updateTaskState(m_activeTaskId);
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
            m_taskTable->setItem(row, 4, new QTableWidgetItem("create"));
            
            // 操作按钮
            QWidget *btnWidget = new QWidget();
            QHBoxLayout *layout = new QHBoxLayout(btnWidget);
            layout->setContentsMargins(5, 2, 5, 2);
            layout->setSpacing(10);
            
            QPushButton *btnStart = new QPushButton("开始");
            QPushButton *btnEnd = new QPushButton("结束");
            QPushButton *btnDelete = new QPushButton("删除");

            btnStart->setObjectName("btnStart");
            btnEnd->setObjectName("btnEnd");
            btnDelete->setObjectName("btnDelete");
            
            // 存入 ID
            btnStart->setProperty("taskId", taskId);
            btnEnd->setProperty("taskId", taskId);
            btnDelete->setProperty("taskId", taskId);
            
            connect(btnStart, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
            connect(btnEnd, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
            connect(btnDelete, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
            
            layout->addWidget(btnStart);
            layout->addWidget(btnEnd);
            layout->addWidget(btnDelete);
            m_taskTable->setCellWidget(row, 5, btnWidget);
        }
    }
    
    // 刷新所有按钮状态
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QWidget *w = m_taskTable->cellWidget(i, 5);
        if (!w) continue;
        
        QPushButton *btnStart = w->findChild<QPushButton*>("btnStart");
        QPushButton *btnEnd = w->findChild<QPushButton*>("btnEnd");
        QPushButton *btnDelete = w->findChild<QPushButton*>("btnDelete");
        if (!btnDelete) continue;
        
        int rowTaskId = m_taskTable->item(i, 0)->text().toInt();
        QString status = m_taskTable->item(i, 4)->text();

        if (status == "create") {
            if (btnStart) btnStart->setVisible(true);
            if (btnEnd) btnEnd->setVisible(false);
            btnDelete->setVisible(true);
        } else if (status == "starting") {
            if (btnStart) btnStart->setVisible(false);
            if (btnEnd) btnEnd->setVisible(true);
            btnDelete->setVisible(true);
        } else {
            if (btnStart) btnStart->setVisible(false);
            if (btnEnd) btnEnd->setVisible(false);
            btnDelete->setVisible(true);
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
        for (int i = 0; i < m_taskTable->rowCount(); ++i) {
            if (m_taskTable->item(i, 0)->text().toInt() == taskId) {
                m_taskTable->item(i, 4)->setText("starting");
                break;
            }
        }
        emit startTaskClicked(taskId);
    } else if (text == "结束") {
        for (int i = 0; i < m_taskTable->rowCount(); ++i) {
            if (m_taskTable->item(i, 0)->text().toInt() == taskId) {
                m_taskTable->item(i, 4)->setText("stop");
                break;
            }
        }
        emit endTaskClicked(taskId);
    } else if (text == "删除") {
        emit deleteTaskClicked(taskId);
    }
    updateTaskState(m_activeTaskId);
}
