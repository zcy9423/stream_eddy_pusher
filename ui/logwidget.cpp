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
    filterLayout->addWidget(m_btnExport);
    
    mainLayout->addLayout(filterLayout);

    // 使用分割器上下排列
    QSplitter *splitter = new QSplitter(Qt::Vertical, this);

    // --- 上半部分：任务列表 ---
    QWidget *topWidget = new QWidget();
    QVBoxLayout *topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0,0,0,0);
    topLayout->addWidget(new QLabel("任务历史记录 (DetectionTask):"));
    
    m_taskView = new QTableView();
    m_taskView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_taskView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_taskView->setAlternatingRowColors(true);
    m_taskView->horizontalHeader()->setStretchLastSection(true);
    topLayout->addWidget(m_taskView);
    
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
    connect(m_taskView, &QTableView::clicked, this, &LogWidget::onTaskSelected);
    connect(m_btnQuery, &QPushButton::clicked, this, &LogWidget::onQueryClicked);
    connect(m_btnExport, &QPushButton::clicked, this, &LogWidget::onExportClicked);
}

void LogWidget::setModels(QSqlTableModel *taskModel, QSqlTableModel *logModel)
{
    m_taskModel = taskModel;
    m_logModel = logModel;

    m_taskView->setModel(m_taskModel);
    m_logView->setModel(m_logModel);
    
    // 初始设置过滤器为空，不显示任何运动详情，避免一启动就刷屏
    m_logModel->setFilter("1=0"); 
}

void LogWidget::refresh()
{
    if (m_taskModel) m_taskModel->select();
    
    // logModel 不需要在这里全局刷新，它依赖于 filter
    if (m_logModel) {
        m_logModel->select();
        // 自动拉取更多数据 (解决默认只显示256行的问题)
        while(m_logModel->canFetchMore()) {
            m_logModel->fetchMore();
        }
    }
}

void LogWidget::onTaskSelected(const QModelIndex &index)
{
    if (!index.isValid() || !m_taskModel || !m_logModel) return;

    // 获取选中行的 ID (假设第一列是 ID)
    int row = index.row();
    QModelIndex idIndex = m_taskModel->index(row, 0); // Column 0 is ID
    int taskId = m_taskModel->data(idIndex).toInt();

    // 过滤日志表
    m_logModel->setFilter(QString("task_id = %1").arg(taskId));
    m_logModel->select();
    
    // 自动拉取更多数据
    while(m_logModel->canFetchMore()) {
        m_logModel->fetchMore();
    }
    
    // 启用导出按钮
    m_btnExport->setEnabled(true);
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
    
    // 清空日志视图
    m_logModel->setFilter("1=0");
    m_logModel->select();
    m_btnExport->setEnabled(false);
}

void LogWidget::onExportClicked()
{
    if (!m_logModel || m_logModel->rowCount() == 0) {
        QMessageBox::warning(this, "导出失败", "当前没有可导出的日志数据。");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "导出日志", "", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法打开文件进行写入：" + file.errorString());
        return;
    }

    QTextStream out(&file);
    // 写入 BOM 防止 Excel 中文乱码
    out << QString::fromUtf8("\xEF\xBB\xBF"); 

    // 1. 写入表头
    QStringList headers;
    for (int i = 0; i < m_logModel->columnCount(); ++i) {
        headers << m_logModel->headerData(i, Qt::Horizontal).toString();
    }
    out << headers.join(",") << "\n";

    // 2. 写入数据
    // 确保拉取所有数据
    while (m_logModel->canFetchMore()) {
        m_logModel->fetchMore();
    }

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
    QMessageBox::information(this, "导出成功", QString("已成功导出 %1 条记录。").arg(rows));
}
