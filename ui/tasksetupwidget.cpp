#include "tasksetupwidget.h"
#include "taskconfigwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QDebug>
#include <QSqlTableModel>
#include <QMessageBox>
#include <QCheckBox>

TaskSetupWidget::TaskSetupWidget(QWidget *parent)
    : QGroupBox("任务配置管理", parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 40, 30, 30);

    // 1. 输入表单
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(4);
    
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
    m_taskTable->setColumnCount(7);
    m_taskTable->setHorizontalHeaderLabels({"选择", "任务ID", "创建时间", "操作员", "管道编号", "当前状态", "操作"});
    m_taskTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_taskTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // 选择列
    m_taskTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // 任务ID列
    m_taskTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents); // 状态列
    m_taskTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    mainLayout->addWidget(m_taskTable);

    // 3. 批量操作按钮
    QHBoxLayout *batchLayout = new QHBoxLayout();
    
    m_btnSelectAll = new QPushButton("全选", this);
    m_btnSelectAll->setObjectName("btnSecondary");
    m_btnSelectAll->setMaximumWidth(80);
    
    m_btnSelectNone = new QPushButton("取消全选", this);
    m_btnSelectNone->setObjectName("btnSecondary");
    m_btnSelectNone->setMaximumWidth(80);
    
    m_btnDeleteSelected = new QPushButton("删除选中", this);
    m_btnDeleteSelected->setObjectName("btnDanger");
    m_btnDeleteSelected->setMaximumWidth(80);
    m_btnDeleteSelected->setEnabled(false);
    
    batchLayout->addWidget(m_btnSelectAll);
    batchLayout->addWidget(m_btnSelectNone);
    batchLayout->addWidget(m_btnDeleteSelected);
    batchLayout->addStretch();
    
    mainLayout->addLayout(batchLayout);

    // 信号连接
    connect(m_editOperator, &QLineEdit::textChanged, this, &TaskSetupWidget::checkInput);
    connect(m_editTubeId, &QLineEdit::textChanged, this, &TaskSetupWidget::checkInput);
    
    connect(m_btnCreate, &QPushButton::clicked, this, [this](){
        emit createTaskClicked(m_editOperator->text().trimmed(), m_editTubeId->text().trimmed());
        m_editTubeId->clear(); // 清空编号方便下一次输入，操作员通常不变
        checkInput();
    });
    
    // 批量操作按钮连接
    connect(m_btnSelectAll, &QPushButton::clicked, this, &TaskSetupWidget::selectAllTasks);
    connect(m_btnSelectNone, &QPushButton::clicked, this, &TaskSetupWidget::selectNoneTasks);
    connect(m_btnDeleteSelected, &QPushButton::clicked, this, &TaskSetupWidget::deleteSelectedTasks);
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

        // 第0列：复选框
        QCheckBox *checkbox = new QCheckBox();
        checkbox->setProperty("taskId", taskId);
        connect(checkbox, &QCheckBox::checkStateChanged, this, &TaskSetupWidget::onCheckboxStateChanged);
        m_taskTable->setCellWidget(row, 0, checkbox);

        // 其他列数据（索引+1）
        m_taskTable->setItem(row, 1, new QTableWidgetItem(QString::number(taskId)));
        m_taskTable->setItem(row, 2, new QTableWidgetItem(startTime.isValid() ? startTime.toString("yyyy-MM-dd HH:mm:ss") : "-"));
        m_taskTable->setItem(row, 3, new QTableWidgetItem(opName.isEmpty() ? "-" : opName));
        m_taskTable->setItem(row, 4, new QTableWidgetItem(tube.isEmpty() ? "-" : tube));
        m_taskTable->setItem(row, 5, new QTableWidgetItem(status));

        QWidget *btnWidget = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(btnWidget);
        layout->setContentsMargins(5, 2, 5, 2);
        layout->setSpacing(5);

        QPushButton *btnConfig = new QPushButton("配置");
        QPushButton *btnExecute = new QPushButton("执行");
        QPushButton *btnResult = new QPushButton("结果");
        QPushButton *btnDelete = new QPushButton("删除");

        btnConfig->setObjectName("btnConfig");
        btnExecute->setObjectName("btnExecute");
        btnResult->setObjectName("btnResult");
        btnDelete->setObjectName("btnDelete");

        btnConfig->setProperty("taskId", taskId);
        btnExecute->setProperty("taskId", taskId);
        btnResult->setProperty("taskId", taskId);
        btnDelete->setProperty("taskId", taskId);

        connect(btnConfig, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
        connect(btnExecute, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
        connect(btnResult, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
        connect(btnDelete, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);

        layout->addWidget(btnConfig);
        layout->addWidget(btnExecute);
        layout->addWidget(btnResult);
        layout->addWidget(btnDelete);
        m_taskTable->setCellWidget(row, 6, btnWidget);
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
            if (m_taskTable->item(i, 1)->text().toInt() == taskId) { // 任务ID现在在第1列
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            int row = m_taskTable->rowCount();
            m_taskTable->insertRow(row);
            
            // 第0列：复选框
            QCheckBox *checkbox = new QCheckBox();
            checkbox->setProperty("taskId", taskId);
            connect(checkbox, &QCheckBox::checkStateChanged, this, &TaskSetupWidget::onCheckboxStateChanged);
            m_taskTable->setCellWidget(row, 0, checkbox);
            
            // 其他列数据（索引+1）
            m_taskTable->setItem(row, 1, new QTableWidgetItem(QString::number(taskId)));
            m_taskTable->setItem(row, 2, new QTableWidgetItem(QDateTime::currentDateTime().toString("HH:mm:ss")));
            // 使用传入的参数填充，或者用"-"占位（如果未提供）
            m_taskTable->setItem(row, 3, new QTableWidgetItem(opName.isEmpty() ? "-" : opName)); 
            m_taskTable->setItem(row, 4, new QTableWidgetItem(tubeId.isEmpty() ? "-" : tubeId));
            m_taskTable->setItem(row, 5, new QTableWidgetItem("create"));
            
            // 操作按钮
            QWidget *btnWidget = new QWidget();
            QHBoxLayout *layout = new QHBoxLayout(btnWidget);
            layout->setContentsMargins(5, 2, 5, 2);
            layout->setSpacing(5);
            
            QPushButton *btnConfig = new QPushButton("配置");
            QPushButton *btnExecute = new QPushButton("执行");
            QPushButton *btnResult = new QPushButton("结果");
            QPushButton *btnDelete = new QPushButton("删除");

            btnConfig->setObjectName("btnConfig");
            btnExecute->setObjectName("btnExecute");
            btnResult->setObjectName("btnResult");
            btnDelete->setObjectName("btnDelete");
            
            // 存入 ID
            btnConfig->setProperty("taskId", taskId);
            btnExecute->setProperty("taskId", taskId);
            btnResult->setProperty("taskId", taskId);
            btnDelete->setProperty("taskId", taskId);
            
            connect(btnConfig, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
            connect(btnExecute, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
            connect(btnResult, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
            connect(btnDelete, &QPushButton::clicked, this, &TaskSetupWidget::onTableBtnClicked);
            
            layout->addWidget(btnConfig);
            layout->addWidget(btnExecute);
            layout->addWidget(btnResult);
            layout->addWidget(btnDelete);
            m_taskTable->setCellWidget(row, 6, btnWidget); // 操作列现在是第6列
        }
    }
    
    // 刷新所有按钮状态
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QWidget *w = m_taskTable->cellWidget(i, 6); // 操作列现在是第6列
        if (!w) continue;
        
        QPushButton *btnConfig = w->findChild<QPushButton*>("btnConfig");
        QPushButton *btnExecute = w->findChild<QPushButton*>("btnExecute");
        QPushButton *btnResult = w->findChild<QPushButton*>("btnResult");
        QPushButton *btnDelete = w->findChild<QPushButton*>("btnDelete");
        if (!btnDelete) continue;
        
        int rowTaskId = m_taskTable->item(i, 1)->text().toInt(); // 任务ID现在在第1列
        QString status = m_taskTable->item(i, 5)->text(); // 状态现在在第5列

        // 根据状态控制按钮可见性
        if (status == "create") {
            if (btnConfig) btnConfig->setEnabled(true);
            if (btnExecute) {
                btnExecute->setEnabled(false);
                btnExecute->setText("执行");
            }
            if (btnResult) btnResult->setEnabled(false);
            btnDelete->setEnabled(true);
        } else if (status == "configured") {
            if (btnConfig) btnConfig->setEnabled(true);
            if (btnExecute) {
                btnExecute->setEnabled(true);
                btnExecute->setText("执行");
            }
            if (btnResult) btnResult->setEnabled(false);
            btnDelete->setEnabled(true);
        } else if (status == "running") {
            if (btnConfig) btnConfig->setEnabled(false);
            if (btnExecute) {
                btnExecute->setEnabled(true);
                btnExecute->setText("停止");
            }
            if (btnResult) btnResult->setEnabled(false);
            btnDelete->setEnabled(false);
        } else if (status == "completed") {
            if (btnConfig) btnConfig->setEnabled(false);
            if (btnExecute) {
                btnExecute->setEnabled(false);
                btnExecute->setText("执行");
            }
            if (btnResult) btnResult->setEnabled(true);
            btnDelete->setEnabled(true);
        } else if (status == "failed") {
            if (btnConfig) btnConfig->setEnabled(true);  // 失败后可以重新配置
            if (btnExecute) {
                btnExecute->setEnabled(true); // 失败后可以重新执行
                btnExecute->setText("执行");
            }
            if (btnResult) btnResult->setEnabled(true);   // 可以查看失败原因
            btnDelete->setEnabled(true);
        } else if (status == "stopped") {
            if (btnConfig) btnConfig->setEnabled(true);  // 停止后可以重新配置
            if (btnExecute) {
                btnExecute->setEnabled(true); // 停止后可以重新执行
                btnExecute->setText("执行");
            }
            if (btnResult) btnResult->setEnabled(true);   // 可以查看停止原因
            btnDelete->setEnabled(true);
        } else {
            // 其他状态（如stop等）
            if (btnConfig) btnConfig->setEnabled(false);
            if (btnExecute) {
                btnExecute->setEnabled(false);
                btnExecute->setText("执行");
            }
            if (btnResult) btnResult->setEnabled(false);
            btnDelete->setEnabled(true);
        }
    }
}

void TaskSetupWidget::updateTaskStatusInTable(int taskId, const QString& status)
{
    // 在表格中找到对应的任务并更新状态
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        if (m_taskTable->item(i, 1)->text().toInt() == taskId) { // 任务ID现在在第1列
            m_taskTable->item(i, 5)->setText(status); // 状态现在在第5列
            break;
        }
    }
    // 更新按钮状态
    updateTaskState(m_activeTaskId);
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
    
    if (text == "配置") {
        emit configTaskClicked(taskId);
    } else if (text == "执行") {
        // 不在这里更新状态，让MainWindow在设备连接检查通过后再更新
        emit executeTaskClicked(taskId);
    } else if (text == "停止") {
        emit stopTaskClicked(taskId);
    } else if (text == "结果") {
        emit viewResultClicked(taskId);
    } else if (text == "删除") {
        emit deleteTaskClicked(taskId);
    }
    updateTaskState(m_activeTaskId);
}
void TaskSetupWidget::selectAllTasks()
{
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(m_taskTable->cellWidget(i, 0));
        if (checkbox) {
            checkbox->setChecked(true);
        }
    }
}

void TaskSetupWidget::selectNoneTasks()
{
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(m_taskTable->cellWidget(i, 0));
        if (checkbox) {
            checkbox->setChecked(false);
        }
    }
}

void TaskSetupWidget::deleteSelectedTasks()
{
    QList<int> selectedTaskIds;
    
    // 收集选中的任务ID
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(m_taskTable->cellWidget(i, 0));
        if (checkbox && checkbox->isChecked()) {
            int taskId = checkbox->property("taskId").toInt();
            selectedTaskIds.append(taskId);
        }
    }
    
    if (selectedTaskIds.isEmpty()) {
        QMessageBox::information(this, "提示", "请先选择要删除的任务");
        return;
    }
    
    const auto reply = QMessageBox::question(
        this,
        "确认删除",
        QString("确认删除选中的 %1 个任务？").arg(selectedTaskIds.size()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // 发出批量删除信号
        emit batchDeleteTasksClicked(selectedTaskIds);
    }
}

void TaskSetupWidget::onCheckboxStateChanged()
{
    // 检查是否有选中的任务，控制删除按钮状态
    bool hasSelected = false;
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(m_taskTable->cellWidget(i, 0));
        if (checkbox && checkbox->isChecked()) {
            hasSelected = true;
            break;
        }
    }
    
    m_btnDeleteSelected->setEnabled(hasSelected);
}