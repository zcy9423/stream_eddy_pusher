#include "connectionwidget.h"
#include "../core/configmanager.h"

ConnectionWidget::ConnectionWidget(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel *lblTitle = new QLabel("运动控制仪表盘", this);
    lblTitle->setObjectName("AppTitle");

    m_portCombo = new QComboBox(this);
    m_portCombo->setMinimumWidth(160);
    m_portCombo->setFixedHeight(35);
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        m_portCombo->addItem(info.portName());
    }
    m_portCombo->addItem("COM_VIRTUAL");

    m_baudCombo = new QComboBox(this);
    m_baudCombo->setMinimumWidth(100);
    m_baudCombo->setFixedHeight(35);
    QStringList baudRates = {"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"};
    m_baudCombo->addItems(baudRates);
    // 从配置加载默认波特率
    int defaultBaud = ConfigManager::instance().serialBaudRate();
    m_baudCombo->setCurrentText(QString::number(defaultBaud));

    m_btnConnect = new QPushButton("连接设备", this);
    m_btnConnect->setCursor(Qt::PointingHandCursor);
    m_btnConnect->setObjectName("btnConnect");
    m_btnConnect->setFixedSize(100, 35);

    layout->addWidget(lblTitle);
    layout->addStretch();
    layout->addWidget(new QLabel("端口:"));
    layout->addWidget(m_portCombo);
    layout->addSpacing(15);
    layout->addWidget(new QLabel("波特率:"));
    layout->addWidget(m_baudCombo);
    layout->addWidget(m_btnConnect);

    connect(m_btnConnect, &QPushButton::clicked, this, &ConnectionWidget::connectClicked);
}

QString ConnectionWidget::currentPort() const
{
    return m_portCombo->currentText();
}

int ConnectionWidget::currentBaud() const
{
    return m_baudCombo->currentText().toInt();
}

void ConnectionWidget::setConnectedState(bool connected)
{
    m_portCombo->setEnabled(!connected);
    m_baudCombo->setEnabled(!connected);
    m_btnConnect->setText(connected ? "断开连接" : "连接设备");
}
