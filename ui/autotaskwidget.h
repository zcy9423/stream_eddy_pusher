#ifndef AUTOTASKWIDGET_H
#define AUTOTASKWIDGET_H

#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
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
    void pauseTaskClicked();
    void resumeTaskClicked();
    void stopTaskClicked();

private:
    QDoubleSpinBox *m_spinMinPos;
    QDoubleSpinBox *m_spinMaxPos;
    QDoubleSpinBox *m_spinAutoSpeed;
    QSpinBox *m_spinCycles;
    QProgressBar *m_progressBar;
    QLabel *m_lblCycleCount;
    QPushButton *m_btnStartTask;
    QPushButton *m_btnPauseTask;
    
    bool m_isPaused = false;
};

#endif // AUTOTASKWIDGET_H
