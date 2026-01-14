#ifndef TASKCONFIGDIALOG_H
#define TASKCONFIGDIALOG_H

#include <QDialog>
#include <QLineEdit>

/**
 * @brief 任务配置对话框
 * 
 * 用于在开始新的检测任务前，收集操作员信息和管道编号。
 * 只有输入有效信息后，才允许确认。
 */
class TaskConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TaskConfigDialog(QWidget *parent = nullptr);

    /**
     * @brief 获取输入的操作员姓名
     */
    QString operatorName() const;

    /**
     * @brief 获取输入的管道编号
     */
    QString tubeId() const;

private:
    QLineEdit *m_editOperator;
    QLineEdit *m_editTubeId;
    QPushButton *m_btnOk;

    /**
     * @brief 检查输入有效性
     * 若两者都不为空，则启用确定按钮
     */
    void checkInput();
};

#endif // TASKCONFIGDIALOG_H
