#ifndef STATUSWIDGET_H
#define STATUSWIDGET_H

#include <QGroupBox>
#include <QLabel>
#include "../communication/protocol.h"

class StatusWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit StatusWidget(QWidget *parent = nullptr);
    void updateStatus(const MotionFeedback &fb);

private:
    QLabel *m_lblPos;
    QLabel *m_lblSpeed;
    QLabel *m_lblStatus;
    
    // 指示灯
    QLabel *m_ledLeftLimit;
    QLabel *m_ledRightLimit;
    QLabel *m_ledEmergency;
};

#endif // STATUSWIDGET_H
