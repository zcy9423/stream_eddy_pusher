#ifndef TASKSETUPWIDGET_H
#define TASKSETUPWIDGET_H

#include <QWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

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
     */
    void updateTaskState(int taskId);

    QString operatorName() const;
    QString tubeId() const;

signals:
    void startTaskClicked();
    void endTaskClicked();

private:
    QLineEdit *m_editOperator;
    QLineEdit *m_editTubeId;
    QPushButton *m_btnStart;
    QPushButton *m_btnEnd;
    QLabel *m_lblCurrentStatus;

private slots:
    void checkInput();
};

#endif // TASKSETUPWIDGET_H
