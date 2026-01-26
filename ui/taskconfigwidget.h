#ifndef TASKCONFIGWIDGET_H
#define TASKCONFIGWIDGET_H

#include <QDialog>
#include <QTabWidget>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "../core/taskmanager.h"

/**
 * @brief 任务配置对话框
 * 
 * 允许用户为任务配置执行步骤，包括：
 * 1. 自动往返扫描配置
 * 2. 高级脚本序列配置
 */
class TaskConfigWidget : public QDialog
{
    Q_OBJECT

public:
    explicit TaskConfigWidget(int taskId, QWidget *parent = nullptr);

    /**
     * @brief 获取配置的任务类型
     * @return "auto_scan" 或 "sequence"
     */
    QString getTaskType() const;

    /**
     * @brief 获取任务配置JSON字符串
     */
    QString getTaskConfig() const;

    /**
     * @brief 设置任务配置
     * @param taskType 任务类型
     * @param taskConfig 任务配置JSON
     */
    void setTaskConfig(const QString &taskType, const QString &taskConfig);

private slots:
    void onAddStep();
    void onRemoveStep();
    void onClearSteps();
    void onTabChanged(int index);

private:
    void setupAutoScanTab(QWidget *tab);
    void setupSequenceTab(QWidget *tab);
    void loadConfigFromJson(const QString &taskType, const QString &configJson);
    QString generateConfigJson() const;

private:
    int m_taskId;
    QTabWidget *m_tabWidget;

    // 自动扫描配置
    QDoubleSpinBox *m_spinMinPos;
    QDoubleSpinBox *m_spinMaxPos;
    QDoubleSpinBox *m_spinSpeed;
    QSpinBox *m_spinCycles;

    // 脚本序列配置
    QTableWidget *m_stepsTable;
    QComboBox *m_comboStepType;
    QDoubleSpinBox *m_spinParam1;
    QDoubleSpinBox *m_spinParam2;
    QSpinBox *m_spinSeqCycles;

    QPushButton *m_btnOk;
    QPushButton *m_btnCancel;
};

#endif // TASKCONFIGWIDGET_H