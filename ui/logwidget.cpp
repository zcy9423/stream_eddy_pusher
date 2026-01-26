#include "logwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QSplitter>
#include <QDate>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QSqlRecord>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QCheckBox>
#include <QDebug>

LogWidget::LogWidget(QWidget *parent) : QGroupBox("数据日志 (SQLite)", parent)
{
    // 移除阴影
    // QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect; ...
    // this->setGraphicsEffect(shadow);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // --- 顶部筛选栏 ---
    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(10);
    
    m_dateStart = new QDateEdit(QDate::currentDate());
    m_dateStart->setCalendarPopup(true);
    m_dateStart->setDisplayFormat("yyyy-MM-dd");
    
    m_dateEnd = new QDateEdit(QDate::currentDate());
    m_dateEnd->setCalendarPopup(true);
    m_dateEnd->setDisplayFormat("yyyy-MM-dd");
    
    m_editTubeId = new QLineEdit();
    m_editTubeId->setPlaceholderText("管号过滤 (可选)");
    m_editTubeId->setFixedWidth(150);
    
    m_btnQuery = new QPushButton("查询任务");
    m_btnQuery->setCursor(Qt::PointingHandCursor);
    m_btnQuery->setObjectName("btnAction"); // 复用样式
    
    m_btnSelectAll = new QPushButton("全选/取消");
    m_btnSelectAll->setCursor(Qt::PointingHandCursor);
    m_btnSelectAll->setEnabled(false);
    
    m_btnExport = new QPushButton("导出选中任务详情");
    m_btnExport->setCursor(Qt::PointingHandCursor);
    m_btnExport->setObjectName("btnConnect"); // 复用样式
    m_btnExport->setEnabled(false); // 初始禁用，选中任务后启用
    
    filterLayout->addWidget(new QLabel("开始日期\nStart:"));
    filterLayout->addWidget(m_dateStart);
    filterLayout->addWidget(new QLabel("结束日期\nEnd:"));
    filterLayout->addWidget(m_dateEnd);
    filterLayout->addWidget(new QLabel("管号\nTube ID:"));
    filterLayout->addWidget(m_editTubeId);
    filterLayout->addWidget(m_btnQuery);
    filterLayout->addStretch();
    filterLayout->addWidget(m_btnSelectAll);
    filterLayout->addWidget(m_btnExport);
    
    mainLayout->addLayout(filterLayout);

    // 使用分割器上下排列
    QSplitter *splitter = new QSplitter(Qt::Vertical, this);

    // --- 上半部分：任务列表 ---
    QWidget *topWidget = new QWidget();
    QVBoxLayout *topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0,0,0,0);
    topLayout->addWidget(new QLabel("任务历史记录 (DetectionTask) - 勾选要导出的任务:"));
    
    m_taskTable = new QTableWidget();
    m_taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_taskTable->setAlternatingRowColors(true);
    m_taskTable->horizontalHeader()->setStretchLastSection(true);
    topLayout->addWidget(m_taskTable);
    
    splitter->addWidget(topWidget);

    // --- 下半部分：详细日志 ---
    QWidget *bottomWidget = new QWidget();
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0,0,0,0);
    bottomLayout->addWidget(new QLabel("选中任务的运动详情 (MotionLog):"));

    m_logView = new QTableView();
    m_logView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logView->setAlternatingRowColors(true);
    m_logView->horizontalHeader()->setStretchLastSection(true);
    bottomLayout->addWidget(m_logView);

    splitter->addWidget(bottomWidget);
    
    // 设置初始比例
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter);

    // 连接信号
    connect(m_taskTable, &QTableWidget::cellClicked, this, &LogWidget::onTaskSelected);
    connect(m_btnQuery, &QPushButton::clicked, this, &LogWidget::onQueryClicked);
    connect(m_btnExport, &QPushButton::clicked, this, &LogWidget::onExportClicked);
    connect(m_btnSelectAll, &QPushButton::clicked, this, &LogWidget::onSelectAllClicked);
}

void LogWidget::setModels(QSqlTableModel *taskModel, QSqlTableModel *logModel)
{
    m_taskModel = taskModel;
    m_logModel = logModel;

    // 设置日志视图模型
    m_logView->setModel(m_logModel);
    
    // 初始设置过滤器为空，不显示任何运动详情，避免一启动就刷屏
    m_logModel->setFilter("1=0"); 
    
    // 更新任务表格
    updateTaskTable();
}

void LogWidget::refresh()
{
    if (m_taskModel) m_taskModel->select();
    
    // 保存当前复选框状态
    QSet<int> selectedTaskIds = getSelectedTaskIds();
    
    // 更新任务表格
    updateTaskTable();
    
    // 恢复复选框状态
    restoreCheckboxStates(selectedTaskIds);
    
    // logModel 不需要在这里全局刷新，它依赖于 filter
    if (m_logModel) {
        m_logModel->select();
        // 自动拉取更多数据 (解决默认只显示256行的问题)
        while(m_logModel->canFetchMore()) {
            m_logModel->fetchMore();
        }
    }
}

void LogWidget::onTaskSelected(int row, int column)
{
    if (row < 0 || !m_taskModel || !m_logModel) return;

    // 如果点击的是复选框列，不处理（复选框有自己的信号）
    if (column == 0) {
        return;
    }

    // 获取选中行的 ID (现在是第二列，因为第一列是复选框)
    QTableWidgetItem *idItem = m_taskTable->item(row, 1); // Column 1 is ID
    if (!idItem) return;
    
    int taskId = idItem->text().toInt();

    // 过滤日志表显示当前点击任务的详情
    m_logModel->setFilter(QString("task_id = %1").arg(taskId));
    m_logModel->select();
    
    // 自动拉取更多数据
    while(m_logModel->canFetchMore()) {
        m_logModel->fetchMore();
    }
}

void LogWidget::onCheckboxStateChanged()
{
    // 更新选中行的背景色
    QCheckBox *senderCheckbox = qobject_cast<QCheckBox*>(sender());
    if (senderCheckbox) {
        // 找到这个复选框所在的行
        for (int i = 0; i < m_taskTable->rowCount(); ++i) {
            QCheckBox *checkbox = qobject_cast<QCheckBox*>(m_taskTable->cellWidget(i, 0));
            if (checkbox == senderCheckbox) {
                // 设置行的背景色
                QColor bgColor = checkbox->isChecked() ? QColor(235, 245, 251) : QColor(255, 255, 255);
                for (int col = 0; col < m_taskTable->columnCount(); ++col) {
                    QTableWidgetItem *item = m_taskTable->item(i, col);
                    if (item) {
                        item->setBackground(bgColor);
                    }
                }
                break;
            }
        }
    }
    
    updateExportButtonState();
}

void LogWidget::onQueryClicked()
{
    if (!m_taskModel) return;

    QString filter = QString("start_time BETWEEN '%1 00:00:00' AND '%2 23:59:59'")
                         .arg(m_dateStart->date().toString("yyyy-MM-dd"))
                         .arg(m_dateEnd->date().toString("yyyy-MM-dd"));

    QString tubeId = m_editTubeId->text().trimmed();
    if (!tubeId.isEmpty()) {
        filter += QString(" AND tube_id LIKE '%%1%'").arg(tubeId);
    }

    m_taskModel->setFilter(filter);
    m_taskModel->select();
    
    // 更新任务表格
    updateTaskTable();
    
    // 清空日志视图
    m_logModel->setFilter("1=0");
    m_logModel->select();
    
    updateExportButtonState();
}

void LogWidget::onExportClicked()
{
    QSet<int> selectedTaskIds = getSelectedTaskIds();
    if (selectedTaskIds.isEmpty()) {
        QMessageBox::warning(this, "导出失败", "请先选择要导出的任务。");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "导出任务详情", "", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法打开文件进行写入：" + file.errorString());
        return;
    }

    QTextStream out(&file);
    // 写入 BOM 防止 Excel 中文乱码
    out << QString::fromUtf8("\xEF\xBB\xBF"); 

    // 构建任务ID列表的过滤条件
    QStringList taskIdList;
    for (int taskId : selectedTaskIds) {
        taskIdList << QString::number(taskId);
    }
    QString filter = QString("task_id IN (%1)").arg(taskIdList.join(","));
    
    // 临时设置过滤器来获取所有选中任务的日志
    QString originalFilter = m_logModel->filter();
    m_logModel->setFilter(filter);
    m_logModel->select();
    
    // 确保拉取所有数据
    while (m_logModel->canFetchMore()) {
        m_logModel->fetchMore();
    }

    if (m_logModel->rowCount() == 0) {
        QMessageBox::warning(this, "导出失败", "选中的任务没有运动日志数据。");
        file.close();
        m_logModel->setFilter(originalFilter);
        m_logModel->select();
        return;
    }

    // 1. 写入表头
    QStringList headers;
    for (int i = 0; i < m_logModel->columnCount(); ++i) {
        headers << m_logModel->headerData(i, Qt::Horizontal).toString();
    }
    out << headers.join(",") << "\n";

    // 2. 写入数据
    int rows = m_logModel->rowCount();
    for (int i = 0; i < rows; ++i) {
        QStringList rowData;
        for (int j = 0; j < m_logModel->columnCount(); ++j) {
            QString data = m_logModel->index(i, j).data().toString();
            // 处理包含逗号的字段
            if (data.contains(",")) {
                data = "\"" + data + "\"";
            }
            rowData << data;
        }
        out << rowData.join(",") << "\n";
    }

    file.close();
    
    // 恢复原来的过滤器
    m_logModel->setFilter(originalFilter);
    m_logModel->select();
    
    QMessageBox::information(this, "导出成功", 
        QString("已成功导出 %1 个任务的 %2 条运动记录。").arg(selectedTaskIds.size()).arg(rows));
}

void LogWidget::onSelectAllClicked()
{
    // 检查当前是否有任何选中的复选框
    bool hasChecked = false;
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(m_taskTable->cellWidget(i, 0));
        if (checkbox && checkbox->isChecked()) {
            hasChecked = true;
            break;
        }
    }
    
    // 如果有选中的，则全部取消；否则全部选中
    bool newState = !hasChecked;
    
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(m_taskTable->cellWidget(i, 0));
        if (checkbox) {
            checkbox->setChecked(newState);
        }
    }
}

void LogWidget::updateTaskTable()
{
    if (!m_taskModel) return;
    
    // 清空表格
    m_taskTable->clear();
    m_taskTable->setRowCount(0);
    
    // 设置表头
    QStringList headers;
    headers << "选择";
    for (int i = 0; i < m_taskModel->columnCount(); ++i) {
        headers << m_taskModel->headerData(i, Qt::Horizontal).toString();
    }
    m_taskTable->setColumnCount(headers.size());
    m_taskTable->setHorizontalHeaderLabels(headers);
    
    // 复制数据
    for (int row = 0; row < m_taskModel->rowCount(); ++row) {
        m_taskTable->insertRow(row);
        
        // 第0列：复选框
        QCheckBox *checkbox = new QCheckBox();
        // 从第二列获取任务ID（假设第一列是ID）
        QModelIndex idIndex = m_taskModel->index(row, 0);
        int taskId = m_taskModel->data(idIndex).toInt();
        checkbox->setProperty("taskId", taskId);
        connect(checkbox, &QCheckBox::checkStateChanged, this, &LogWidget::onCheckboxStateChanged);
        m_taskTable->setCellWidget(row, 0, checkbox);
        
        // 其他列：从原模型复制数据
        for (int col = 0; col < m_taskModel->columnCount(); ++col) {
            QModelIndex sourceIndex = m_taskModel->index(row, col);
            QTableWidgetItem *item = new QTableWidgetItem(m_taskModel->data(sourceIndex).toString());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable); // 设置为只读
            m_taskTable->setItem(row, col + 1, item); // +1 因为第0列是复选框
        }
    }
    
    // 调整列宽
    m_taskTable->resizeColumnToContents(0); // 复选框列
    m_taskTable->horizontalHeader()->setStretchLastSection(true);
    
    // 启用全选按钮
    m_btnSelectAll->setEnabled(m_taskModel->rowCount() > 0);
}

void LogWidget::updateExportButtonState()
{
    bool hasSelected = false;
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(m_taskTable->cellWidget(i, 0));
        if (checkbox && checkbox->isChecked()) {
            hasSelected = true;
            break;
        }
    }
    m_btnExport->setEnabled(hasSelected);
}

void LogWidget::restoreCheckboxStates(const QSet<int> &selectedTaskIds)
{
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(m_taskTable->cellWidget(i, 0));
        if (checkbox) {
            int taskId = checkbox->property("taskId").toInt();
            bool isSelected = selectedTaskIds.contains(taskId);
            checkbox->setChecked(isSelected);
            
            // 设置行的背景色来指示选中状态
            QColor bgColor = isSelected ? QColor(235, 245, 251) : QColor(255, 255, 255); // 浅蓝色表示选中
            for (int col = 0; col < m_taskTable->columnCount(); ++col) {
                QTableWidgetItem *item = m_taskTable->item(i, col);
                if (item) {
                    item->setBackground(bgColor);
                }
            }
        }
    }
    updateExportButtonState();
}

QSet<int> LogWidget::getSelectedTaskIds() const
{
    QSet<int> selectedIds;
    for (int i = 0; i < m_taskTable->rowCount(); ++i) {
        QCheckBox *checkbox = qobject_cast<QCheckBox*>(m_taskTable->cellWidget(i, 0));
        if (checkbox && checkbox->isChecked()) {
            int taskId = checkbox->property("taskId").toInt();
            selectedIds.insert(taskId);
        }
    }
    return selectedIds;
}
