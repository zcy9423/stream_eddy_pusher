#ifndef CONNECTIONWIDGET_H
#define CONNECTIONWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QSerialPortInfo>

class ConnectionWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConnectionWidget(QWidget *parent = nullptr);

    QString currentPort() const;
    int currentBaud() const;
    void setConnectedState(bool connected);

signals:
    void connectClicked();

private:
    QComboBox *m_portCombo;
    QComboBox *m_baudCombo;
    QPushButton *m_btnConnect;
};

#endif // CONNECTIONWIDGET_H
