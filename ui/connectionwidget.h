#ifndef CONNECTIONWIDGET_H
#define CONNECTIONWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLineEdit>
#include <QSerialPortInfo>

class ConnectionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConnectionWidget(QWidget *parent = nullptr);

    void setConnectedState(bool connected);

signals:
    // type: 0=Serial, 1=Tcp, 2=Sim
    void connectClicked(int type, const QString &addr, int portOrBaud);
    void cancelConnection(); // 新增取消信号

private slots:
    void onModeChanged(int index);
    void onConnectBtnClicked();

private:
    void refreshPorts(); // 刷新串口列表

    QComboBox *m_modeCombo;
    QStackedWidget *m_stack;
    QPushButton *m_btnConnect;
    
    // Serial Page
    QWidget *m_pageSerial;
    QComboBox *m_portCombo;
    QComboBox *m_baudCombo;

    // TCP Page
    QWidget *m_pageTcp;
    QLineEdit *m_ipEdit;
    QLineEdit *m_tcpPortEdit;

    bool m_isConnected = false;
    bool m_isConnecting = false; // 新增连接中状态标志
};

#endif // CONNECTIONWIDGET_H
