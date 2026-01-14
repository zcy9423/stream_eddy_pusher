#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QGroupBox>
#include <QTableView>
#include <QSqlTableModel>

#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>

class LogWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = nullptr);
    
    // 设置数据模型
    void setModels(QSqlTableModel *taskModel, QSqlTableModel *logModel);
    
    // 刷新数据
    void refresh();

private slots:
    // 当在任务列表中选择某行时触发
    void onTaskSelected(const QModelIndex &index);
    
    // 查询按钮点击
    void onQueryClicked();
    
    // 导出按钮点击
    void onExportClicked();

private:
    QTableView *m_taskView; // 任务列表视图
    QTableView *m_logView;  // 详细日志视图
    
    QSqlTableModel *m_taskModel;
    QSqlTableModel *m_logModel;
    
    // 筛选控件
    QDateEdit *m_dateStart;
    QDateEdit *m_dateEnd;
    QLineEdit *m_editTubeId;
    QPushButton *m_btnQuery;
    QPushButton *m_btnExport;
};

#endif // LOGWIDGET_H
