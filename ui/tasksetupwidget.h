#ifndef TASKSETUPWIDGET_H
#define TASKSETUPWIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QComboBox>
#include <QDateEdit>

class QSqlTableModel;

/**
 * @brief 任务配置页面
 * 
 * 允许用户输入操作员和管道信息，并创建/结束任务。
 */
class TaskSetupWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit TaskSetupWidget(QWidget *parent = nullptr);

    /**
     * @brief 更新当前任务状态显示
     * @param taskId 当前任务ID (-1 表示无任务)
     * @param opName 可选：如果提供，将用于填充列表（通常是新任务创建时）
     * @param tubeId 可选：如果提供，将用于填充列表
     */
    void updateTaskState(int taskId, const QString& opName = "", const QString& tubeId = "");
    
    /**
     * @brief 更新表格中指定任务的状态
     * @param taskId 任务ID
     * @param status 新状态
     */
    void updateTaskStatusInTable(int taskId, const QString& status);
    
    void loadHistory(QSqlTableModel *model);

    QString operatorName() const;
    QString tubeId() const;

signals:
    void createTaskClicked(const QString &opName, const QString &tubeId);
    void configTaskClicked(int taskId);
    void executeTaskClicked(int taskId);
    void stopTaskClicked(int taskId);
    void viewResultClicked(int taskId);
    void deleteTaskClicked(int taskId);
    void batchDeleteTasksClicked(const QList<int> &taskIds);

private slots:
    void checkInput();
    void onTableBtnClicked();
    void selectAllTasks();
    void selectNoneTasks();
    void deleteSelectedTasks();
    void onCheckboxStateChanged();
    
    // 新增：高级筛选相关槽函数
    void onSearchTextChanged();
    void onFilterChanged();
    void onResetFilters();
    void onAdvancedFilterToggled();

private:
    void applyFilters();
    void populateFilteredTable();
    bool matchesFilters(int row) const;

private:
    QLineEdit *m_editOperator;
    QLineEdit *m_editTubeId;
    QPushButton *m_btnCreate;
    
    // 新增：搜索和筛选控件
    QLineEdit *m_searchEdit;
    QPushButton *m_btnAdvancedFilter;
    QWidget *m_advancedFilterWidget;
    QLineEdit *m_filterTaskId;
    QDateEdit *m_filterStartDate;
    QDateEdit *m_filterEndDate;
    QComboBox *m_filterOperator;
    QLineEdit *m_filterTubeId;
    QComboBox *m_filterStatus;
    QPushButton *m_btnResetFilters;
    
    // 任务列表相关
    QTableWidget *m_taskTable;
    
    // 批量操作按钮
    QPushButton *m_btnSelectAll;
    QPushButton *m_btnSelectNone;
    QPushButton *m_btnDeleteSelected;
    
    // 当前活跃任务ID
    int m_activeTaskId = -1;
    
    // 数据存储
    QSqlTableModel *m_model = nullptr;
    QList<QList<QVariant>> m_allTaskData; // 存储所有任务数据用于筛选
};

#endif // TASKSETUPWIDGET_H
