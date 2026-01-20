#ifndef TASKSETUPWIDGET_H
#define TASKSETUPWIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>

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
    void loadHistory(QSqlTableModel *model);

    QString operatorName() const;
    QString tubeId() const;

signals:
    void createTaskClicked(const QString &opName, const QString &tubeId);
    void startTaskClicked(int taskId);
    void endTaskClicked(int taskId);
    void deleteTaskClicked(int taskId);

private:
    QLineEdit *m_editOperator;
    QLineEdit *m_editTubeId;
    QPushButton *m_btnCreate;
    
    // 任务列表相关
    QTableWidget *m_taskTable;
    
    // 当前活跃任务ID
    int m_activeTaskId = -1;

private slots:
    void checkInput();
    void onTableBtnClicked();
};

#endif // TASKSETUPWIDGET_H
