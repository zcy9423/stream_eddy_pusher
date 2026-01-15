#ifndef STATUSWIDGET_H
#define STATUSWIDGET_H

#include <QGroupBox>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include "../communication/protocol.h"

class StatusWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit StatusWidget(QWidget *parent = nullptr);
    void updateStatus(const MotionFeedback &fb);

private:
    void initChart();

    QLabel *m_lblPos;
    QLabel *m_lblSpeed;
    QLabel *m_lblStatus;
    
    // 指示灯
    QLabel *m_ledLeftLimit;
    QLabel *m_ledRightLimit;
    QLabel *m_ledEmergency;

    // 图表相关
    QChart *m_chart;
    QLineSeries *m_seriesPos;
    QLineSeries *m_seriesSpeed;
    QValueAxis *m_axisX;
    QValueAxis *m_axisYPos;
    QValueAxis *m_axisYSpeed;
    qint64 m_startTime;
    bool m_isRecording = false;
};

#endif // STATUSWIDGET_H
