#include "taskconfigwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

TaskConfigWidget::TaskConfigWidget(int taskId, QWidget *parent)
    : QDialog(parent), m_taskId(taskId)
{
    setWindowTitle(QString("任务配置 - ID: %1").arg(taskId));
    setModal(true);
    resize(800, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 创建选项卡
    m_tabWidget = new QTabWidget();
    
    QWidget *autoScanTab = new QWidget();
    QWidget *sequenceTab = new QWidget();
    
    setupAutoScanTab(autoScanTab);
    setupSequenceTab(sequenceTab);
    
    m_tabWidget->addTab(autoScanTab, "自动往返扫描");
    m_tabWidget->addTab(sequenceTab, "高级脚本序列");
    
    mainLayout->addWidget(m_tabWidget);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_btnOk = new QPushButton("确定");
    m_btnCancel = new QPushButton("取消");
    
    m_btnOk->setObjectName("btnAction");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_btnOk);
    buttonLayout->addWidget(m_btnCancel);
    
    mainLayout->addLayout(buttonLayout);

    // 信号连接
    connect(m_btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &TaskConfigWidget::onTabChanged);
}

void TaskConfigWidget::setupAutoScanTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(20);
    layout->setContentsMargins(30, 30, 30, 30);

    QFormLayout *formLayout = new QFormLayout();
    
    m_spinMinPos = new QDoubleSpinBox();
    m_spinMinPos->setRange(0, 1000);
    m_spinMinPos->setSuffix(" mm");
    m_spinMinPos->setValue(0.0);
    
    m_spinMaxPos = new QDoubleSpinBox();
    m_spinMaxPos->setRange(0, 1000);
    m_spinMaxPos->setSuffix(" mm");
    m_spinMaxPos->setValue(100.0);
    
    m_spinSpeed = new QDoubleSpinBox();
    m_spinSpeed->setRange(1, 100);
    m_spinSpeed->setSuffix(" %");
    m_spinSpeed->setValue(20.0);
    
    m_spinCycles = new QSpinBox();
    m_spinCycles->setRange(1, 9999);
    m_spinCycles->setValue(5);

    formLayout->addRow("起点位置:", m_spinMinPos);
    formLayout->addRow("终点位置:", m_spinMaxPos);
    formLayout->addRow("扫描速度:", m_spinSpeed);
    formLayout->addRow("往返次数:", m_spinCycles);

    layout->addLayout(formLayout);
    layout->addStretch();
}

void TaskConfigWidget::setupSequenceTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(30, 30, 30, 30);
    
    // 步骤列表
    m_stepsTable = new QTableWidget();
    m_stepsTable->setColumnCount(3);
    m_stepsTable->setHorizontalHeaderLabels({"类型", "参数1", "参数2"});
    m_stepsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_stepsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_stepsTable);
    
    // 添加步骤区域
    QHBoxLayout *inputLayout = new QHBoxLayout();
    
    m_comboStepType = new QComboBox();
    m_comboStepType->addItem("移动到 (MoveTo)", static_cast<int>(TaskManager::StepType::MoveTo));
    m_comboStepType->addItem("等待 (Wait)", static_cast<int>(TaskManager::StepType::Wait));
    
    m_spinParam1 = new QDoubleSpinBox();
    m_spinParam1->setRange(0, 99999);
    m_spinParam1->setSuffix(" mm");
    
    m_spinParam2 = new QDoubleSpinBox();
    m_spinParam2->setRange(0, 99999);
    m_spinParam2->setSuffix(" %");
    m_spinParam2->setValue(20.0);
    
    QPushButton *btnAdd = new QPushButton("添加步骤");
    QPushButton *btnRemove = new QPushButton("删除选中");
    QPushButton *btnClear = new QPushButton("清空所有");
    
    inputLayout->addWidget(new QLabel("类型:"));
    inputLayout->addWidget(m_comboStepType);
    inputLayout->addWidget(new QLabel("参数1:"));
    inputLayout->addWidget(m_spinParam1);
    inputLayout->addWidget(new QLabel("参数2:"));
    inputLayout->addWidget(m_spinParam2);
    inputLayout->addWidget(btnAdd);
    inputLayout->addWidget(btnRemove);
    inputLayout->addWidget(btnClear);
    
    layout->addLayout(inputLayout);
    
    // 循环次数
    QHBoxLayout *cycleLayout = new QHBoxLayout();
    m_spinSeqCycles = new QSpinBox();
    m_spinSeqCycles->setRange(1, 9999);
    m_spinSeqCycles->setValue(1);
    
    cycleLayout->addWidget(new QLabel("循环次数:"));
    cycleLayout->addWidget(m_spinSeqCycles);
    cycleLayout->addStretch();
    
    layout->addLayout(cycleLayout);

    // 信号连接
    connect(btnAdd, &QPushButton::clicked, this, &TaskConfigWidget::onAddStep);
    connect(btnRemove, &QPushButton::clicked, this, &TaskConfigWidget::onRemoveStep);
    connect(btnClear, &QPushButton::clicked, this, &TaskConfigWidget::onClearSteps);
    
    // 动态更新参数标签
    connect(m_comboStepType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        TaskManager::StepType type = static_cast<TaskManager::StepType>(m_comboStepType->currentData().toInt());
        if (type == TaskManager::StepType::MoveTo) {
            m_spinParam1->setSuffix(" mm");
            m_spinParam2->setVisible(true);
            m_spinParam2->setSuffix(" %");
        } else if (type == TaskManager::StepType::Wait) {
            m_spinParam1->setSuffix(" ms");
            m_spinParam2->setVisible(false);
        }
    });
}

void TaskConfigWidget::onAddStep()
{
    int row = m_stepsTable->rowCount();
    m_stepsTable->insertRow(row);
    
    TaskManager::StepType type = static_cast<TaskManager::StepType>(m_comboStepType->currentData().toInt());
    QString typeStr = m_comboStepType->currentText();
    
    QTableWidgetItem *itemType = new QTableWidgetItem(typeStr);
    itemType->setData(Qt::UserRole, static_cast<int>(type));
    
    QTableWidgetItem *itemP1 = new QTableWidgetItem(QString::number(m_spinParam1->value()));
    QTableWidgetItem *itemP2 = new QTableWidgetItem(QString::number(m_spinParam2->value()));
    
    m_stepsTable->setItem(row, 0, itemType);
    m_stepsTable->setItem(row, 1, itemP1);
    m_stepsTable->setItem(row, 2, itemP2);
}

void TaskConfigWidget::onRemoveStep()
{
    int row = m_stepsTable->currentRow();
    if (row >= 0) {
        m_stepsTable->removeRow(row);
    }
}

void TaskConfigWidget::onClearSteps()
{
    m_stepsTable->setRowCount(0);
}

void TaskConfigWidget::onTabChanged(int index)
{
    // 可以在这里添加标签页切换时的逻辑
}

QString TaskConfigWidget::getTaskType() const
{
    if (m_tabWidget->currentIndex() == 0) {
        return "auto_scan";
    } else {
        return "sequence";
    }
}

QString TaskConfigWidget::getTaskConfig() const
{
    return generateConfigJson();
}

void TaskConfigWidget::setTaskConfig(const QString &taskType, const QString &taskConfig)
{
    if (taskType == "auto_scan") {
        m_tabWidget->setCurrentIndex(0);
    } else if (taskType == "sequence") {
        m_tabWidget->setCurrentIndex(1);
    }
    
    if (!taskConfig.isEmpty()) {
        loadConfigFromJson(taskType, taskConfig);
    }
}

void TaskConfigWidget::loadConfigFromJson(const QString &taskType, const QString &configJson)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(configJson.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        return;
    }
    
    QJsonObject config = doc.object();
    
    if (taskType == "auto_scan") {
        m_spinMinPos->setValue(config["minPos"].toDouble(0.0));
        m_spinMaxPos->setValue(config["maxPos"].toDouble(100.0));
        m_spinSpeed->setValue(config["speed"].toDouble(20.0));
        m_spinCycles->setValue(config["cycles"].toInt(5));
    } else if (taskType == "sequence") {
        m_spinSeqCycles->setValue(config["cycles"].toInt(1));
        
        QJsonArray steps = config["steps"].toArray();
        m_stepsTable->setRowCount(0);
        
        for (const QJsonValue &stepValue : steps) {
            QJsonObject step = stepValue.toObject();
            int row = m_stepsTable->rowCount();
            m_stepsTable->insertRow(row);
            
            int type = step["type"].toInt();
            QString typeStr;
            if (type == static_cast<int>(TaskManager::StepType::MoveTo)) {
                typeStr = "移动到 (MoveTo)";
            } else if (type == static_cast<int>(TaskManager::StepType::Wait)) {
                typeStr = "等待 (Wait)";
            }
            
            QTableWidgetItem *itemType = new QTableWidgetItem(typeStr);
            itemType->setData(Qt::UserRole, type);
            
            QTableWidgetItem *itemP1 = new QTableWidgetItem(QString::number(step["param1"].toDouble()));
            QTableWidgetItem *itemP2 = new QTableWidgetItem(QString::number(step["param2"].toDouble()));
            
            m_stepsTable->setItem(row, 0, itemType);
            m_stepsTable->setItem(row, 1, itemP1);
            m_stepsTable->setItem(row, 2, itemP2);
        }
    }
}

QString TaskConfigWidget::generateConfigJson() const
{
    QJsonObject config;
    
    if (getTaskType() == "auto_scan") {
        config["minPos"] = m_spinMinPos->value();
        config["maxPos"] = m_spinMaxPos->value();
        config["speed"] = m_spinSpeed->value();
        config["cycles"] = m_spinCycles->value();
    } else {
        config["cycles"] = m_spinSeqCycles->value();
        
        QJsonArray steps;
        for (int i = 0; i < m_stepsTable->rowCount(); ++i) {
            QJsonObject step;
            step["type"] = m_stepsTable->item(i, 0)->data(Qt::UserRole).toInt();
            step["param1"] = m_stepsTable->item(i, 1)->text().toDouble();
            step["param2"] = m_stepsTable->item(i, 2)->text().toDouble();
            steps.append(step);
        }
        config["steps"] = steps;
    }
    
    QJsonDocument doc(config);
    return doc.toJson(QJsonDocument::Compact);
}