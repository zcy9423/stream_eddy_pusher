#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QGroupBox>
#include <QTableView>
#include <QTableWidget>
#include <QSqlTableModel>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSet>

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
    void onTaskSelected(int row, int column);
    
    // 复选框状态改变
    void onCheckboxStateChanged();
    
    // 查询按钮点击
    void onQueryClicked();
    
    // 导出按钮点击
    void onExportClicked();
    
    // 全选/取消全选
    void onSelectAllClicked();

private:
    void updateTaskTable();
    void updateExportButtonState();
    void restoreCheckboxStates(const QSet<int> &selectedTaskIds);
    QSet<int> getSelectedTaskIds() const;

private:
    QTableWidget *m_taskTable; // 任务列表表格（使用QTableWidget支持复选框）
    QTableView *m_logView;  // 详细日志视图
    
    QSqlTableModel *m_taskModel;
    QSqlTableModel *m_logModel;
    
    // 筛选控件
    QDateEdit *m_dateStart;
    QDateEdit *m_dateEnd;
    QLineEdit *m_editTubeId;
    QPushButton *m_btnQuery;
    QPushButton *m_btnExport;
    QPushButton *m_btnSelectAll;
};

#endif // LOGWIDGET_H
