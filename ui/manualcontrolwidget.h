#ifndef MANUALCONTROLWIDGET_H
#define MANUALCONTROLWIDGET_H

#include <QGroupBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>

class ManualControlWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit ManualControlWidget(QWidget *parent = nullptr);
    void setControlsEnabled(bool enabled);
    int currentSpeed() const;

signals:
    void moveForwardClicked();
    void moveBackwardClicked();
    void stopClicked();
    void speedChanged(int value);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QSlider *m_speedSlider;
    QLabel *m_lblSpeedVal;
    QPushButton *m_btnFwd;
    QPushButton *m_btnBwd;
    QPushButton *m_btnStop;
};

#endif // MANUALCONTROLWIDGET_H
