#ifndef AUTOTASKWIDGET_H
#define AUTOTASKWIDGET_H

#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>
#include <QComboBox>
#include "../core/taskmanager.h"

class AutoTaskWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit AutoTaskWidget(QWidget *parent = nullptr);
    void updateProgress(int completed, int total);
    void updateState(TaskManager::State state);

signals:
    void startTaskClicked(double min, double max, double speed, int cycles);
    void startSequenceClicked(const QList<TaskManager::TaskStep> &steps, int cycles);
    void pauseTaskClicked();
    void resumeTaskClicked();
    void stopTaskClicked();

private slots:
    void onAddStep();
    void onRemoveStep();
    void onClearSteps();
    void onRunSequence();

private:
    void setupSimpleTab(QWidget *tab);
    void setupAdvancedTab(QWidget *tab);

private:
    // Tab 1: Simple Scan
    QDoubleSpinBox *m_spinMinPos;
    QDoubleSpinBox *m_spinMaxPos;
    QDoubleSpinBox *m_spinAutoSpeed;
    QSpinBox *m_spinCycles;
    
    // Tab 2: Advanced Sequence
    QTableWidget *m_tableSteps;
    QComboBox *m_comboStepType;
    QDoubleSpinBox *m_spinStepParam1;
    QDoubleSpinBox *m_spinStepParam2;
    QSpinBox *m_spinSeqCycles;
    QPushButton *m_btnRunSeq;
    
    // Common
    QProgressBar *m_progressBar;
    QLabel *m_lblCycleCount;
    QPushButton *m_btnStartTask;
    QPushButton *m_btnPauseTask;
    
    bool m_isPaused = false;
};

#endif // AUTOTASKWIDGET_H
