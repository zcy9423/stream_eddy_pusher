#include "connectionwidget.h"
#include "../core/configmanager.h"
#include <QIntValidator>
#include <QGraphicsDropShadowEffect>

ConnectionWidget::ConnectionWidget(QWidget *parent) : QWidget(parent)
{
    // 外部主布局 (Widget 本身)
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 卡片容器 (Frame)
    QFrame *cardFrame = new QFrame(this);
    cardFrame->setObjectName("CardFrame");
    // 移除硬编码样式，使用全局 QSS

    // 移除阴影 (深色模式下不需要，或者由全局样式处理)
    // QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect; ...

    // 卡片内部布局 - 使用 Grid Layout 提升响应式
    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(20);

    // 1. 标题 (去掉 emoji 图标，保持简洁)
    QLabel *lblTitle = new QLabel("通信连接 (Communication)", this);
    lblTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2C3E50;"); // 适配亮色
    
    // 顶部行：标题 + 按钮
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->addWidget(lblTitle);
    topRow->addStretch();
    
    // 连接按钮放在右上角
    m_btnConnect = new QPushButton("连接设备", this);
    m_btnConnect->setCursor(Qt::PointingHandCursor);
    m_btnConnect->setObjectName("btnConnect");
    m_btnConnect->setFixedSize(120, 36);
    // 移除硬编码样式，使用全局 QSS
    
    connect(m_btnConnect, &QPushButton::clicked, this, &ConnectionWidget::onConnectBtnClicked);
    
    topRow->addWidget(m_btnConnect);
    cardLayout->addLayout(topRow);

    // 分隔线
    QFrame *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #E6E9ED;"); // 亮色分割线
    cardLayout->addWidget(line);

    // 2. 配置区域 (使用 Flow Layout 或者 Flex 思想，这里用 HBoxLayout + Stretch)
    // 我们将所有配置项放在一行中，通过 spacing 分隔，这样在宽屏下不会留白太多
    
    QHBoxLayout *configLayout = new QHBoxLayout();
    configLayout->setSpacing(30); // 增加间距，使宽屏下分布更均匀
    
    // 2.1 模式选择
    QVBoxLayout *modeGroup = new QVBoxLayout();
    modeGroup->setSpacing(5);
    QLabel *lblMode = new QLabel("连接模式", this);
    lblMode->setStyleSheet("color: #7F8C8D; font-size: 12px; font-weight: bold;");
    
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem("串口通信 (Serial)");
    m_modeCombo->addItem("以太网 (TCP)");
    m_modeCombo->addItem("仿真模式 (Sim)");
    m_modeCombo->setFixedWidth(160);
    m_modeCombo->setFixedHeight(32);
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ConnectionWidget::onModeChanged);
    
    modeGroup->addWidget(lblMode);
    modeGroup->addWidget(m_modeCombo);
    configLayout->addLayout(modeGroup);

    // 2.2 动态参数区域 (Stack)
    // 为了让 Stack 中的 Widget 也能水平排列，我们需要在 Stack 的子 Widget 里使用 HBoxLayout
    m_stack = new QStackedWidget(this);
    m_stack->setFixedHeight(55); // 高度足够容纳 label + input
    m_stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // --- Page 1: Serial ---
    QWidget *pageSerial = new QWidget(this);
    QHBoxLayout *serialLayout = new QHBoxLayout(pageSerial);
    serialLayout->setContentsMargins(0, 0, 0, 0);
    serialLayout->setSpacing(30); // 参数间距
    
    // 端口
    QVBoxLayout *portGroup = new QVBoxLayout();
    portGroup->setSpacing(5);
    QLabel *lblPort = new QLabel("端口 (Port)", this);
    lblPort->setStyleSheet("color: #7F8C8D; font-size: 12px; font-weight: bold;");
    m_portCombo = new QComboBox(this);
    m_portCombo->setMinimumWidth(120);
    m_portCombo->setFixedHeight(32);
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        m_portCombo->addItem(info.portName());
    }
    portGroup->addWidget(lblPort);
    portGroup->addWidget(m_portCombo);
    serialLayout->addLayout(portGroup);

    // 波特率
    QVBoxLayout *baudGroup = new QVBoxLayout();
    baudGroup->setSpacing(5);
    QLabel *lblBaud = new QLabel("波特率 (Baud)", this);
    lblBaud->setStyleSheet("color: #7F8C8D; font-size: 12px; font-weight: bold;");
    m_baudCombo = new QComboBox(this);
    m_baudCombo->setMinimumWidth(100);
    m_baudCombo->setFixedHeight(32);
    QStringList baudRates = {"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"};
    m_baudCombo->addItems(baudRates);
    int defaultBaud = ConfigManager::instance().serialBaudRate();
    m_baudCombo->setCurrentText(QString::number(defaultBaud));
    baudGroup->addWidget(lblBaud);
    baudGroup->addWidget(m_baudCombo);
    serialLayout->addLayout(baudGroup);
    
    serialLayout->addStretch(); // 填充右侧剩余空间

    // --- Page 2: TCP ---
    QWidget *pageTcp = new QWidget(this);
    QHBoxLayout *tcpLayout = new QHBoxLayout(pageTcp);
    tcpLayout->setContentsMargins(0, 0, 0, 0);
    tcpLayout->setSpacing(30);

    // IP
    QVBoxLayout *ipGroup = new QVBoxLayout();
    ipGroup->setSpacing(5);
    QLabel *lblIp = new QLabel("IP 地址", this);
    lblIp->setStyleSheet("color: #7F8C8D; font-size: 12px; font-weight: bold;");
    m_ipEdit = new QLineEdit(this);
    m_ipEdit->setPlaceholderText("192.168.1.100");
    m_ipEdit->setText("192.168.1.100"); 
    m_ipEdit->setFixedWidth(140);
    m_ipEdit->setFixedHeight(32);
    ipGroup->addWidget(lblIp);
    ipGroup->addWidget(m_ipEdit);
    tcpLayout->addLayout(ipGroup);

    // Port
    QVBoxLayout *tcpPortGroup = new QVBoxLayout();
    tcpPortGroup->setSpacing(5);
    QLabel *lblTcpPort = new QLabel("端口 (Port)", this);
    lblTcpPort->setStyleSheet("color: #7F8C8D; font-size: 12px; font-weight: bold;");
    m_tcpPortEdit = new QLineEdit(this);
    m_tcpPortEdit->setPlaceholderText("8080");
    m_tcpPortEdit->setText("8080"); 
    m_tcpPortEdit->setFixedWidth(80);
    m_tcpPortEdit->setFixedHeight(32);
    m_tcpPortEdit->setValidator(new QIntValidator(1, 65535, this));
    tcpPortGroup->addWidget(lblTcpPort);
    tcpPortGroup->addWidget(m_tcpPortEdit);
    tcpLayout->addLayout(tcpPortGroup);

    tcpLayout->addStretch();

    // --- Page 3: Sim ---
    QWidget *pageSim = new QWidget(this);
    QHBoxLayout *simLayout = new QHBoxLayout(pageSim);
    simLayout->setContentsMargins(0,0,0,0);
    QLabel *lblSim = new QLabel("无需配置参数，直接连接即可启动仿真。", this);
    lblSim->setStyleSheet("color: #95A5A6; font-style: italic; margin-top: 15px;"); // 适配亮色
    simLayout->addWidget(lblSim);
    simLayout->addStretch();

    // Add pages
    m_stack->addWidget(pageSerial);
    m_stack->addWidget(pageTcp);
    m_stack->addWidget(pageSim);

    // 将 Stack 添加到配置布局
    configLayout->addWidget(m_stack);
    
    // 整体布局添加
    cardLayout->addLayout(configLayout);
    
    // 如果是小屏，可能需要垂直排列，但 Qt Layout 会自动处理，只要 setMinimumWidth 设置合理
    // 此处设计为横向流式，适应性较好
    
    mainLayout->addWidget(cardFrame);
    
    // 初始化页面
    m_pageSerial = pageSerial;
    m_pageTcp = pageTcp;
}

void ConnectionWidget::onModeChanged(int index)
{
    if (index == 0) m_stack->setCurrentIndex(0);
    else if (index == 1) m_stack->setCurrentIndex(1);
    else m_stack->setCurrentIndex(2);
}

void ConnectionWidget::onConnectBtnClicked()
{
    // 如果已经在连接中(按钮被禁用)，直接返回，防止重复点击
    if (!m_btnConnect->isEnabled()) return;

    int type = m_modeCombo->currentIndex(); // 0=Serial, 1=Tcp, 2=Sim
    QString addr;
    int portOrBaud = 0;

    if (type == 0) { // Serial
        addr = m_portCombo->currentText();
        portOrBaud = m_baudCombo->currentText().toInt();
    } else if (type == 1) { // TCP
        addr = m_ipEdit->text();
        portOrBaud = m_tcpPortEdit->text().toInt();
    }
    
    // 如果当前是未连接状态，点击后进入"连接中"状态
    if (!m_isConnected) {
        m_btnConnect->setText("连接中...");
        m_btnConnect->setEnabled(false); // 禁用按钮防止重复点击
        m_modeCombo->setEnabled(false);
        m_stack->setEnabled(false);
    } else {
        // 如果是已连接状态，点击则是断开连接，也暂时禁用按钮直到状态改变
        m_btnConnect->setEnabled(false);
    }
    
    emit connectClicked(type, addr, portOrBaud);
}

void ConnectionWidget::setConnectedState(bool connected)
{
    m_isConnected = connected;
    
    // 无论连接成功还是失败，都需要重新启用按钮
    m_btnConnect->setEnabled(true);
    
    m_modeCombo->setEnabled(!connected);
    m_stack->setEnabled(!connected);
    
    if (connected) {
        m_btnConnect->setText("断开连接");
        m_btnConnect->setStyleSheet(R"(
            QPushButton#btnConnect {
                background-color: #E74C3C;
                color: white;
                border-radius: 4px;
                font-size: 14px;
                font-weight: bold;
                border: none;
            }
            QPushButton#btnConnect:hover { background-color: #C0392B; }
        )");
    } else {
        m_btnConnect->setText("连接设备");
        m_btnConnect->setStyleSheet(R"(
            QPushButton#btnConnect {
                background-color: #3498DB;
                color: white;
                border-radius: 4px;
                font-size: 14px;
                font-weight: bold;
                border: none;
            }
            QPushButton#btnConnect:hover { background-color: #2980B9; }
        )");
    }
}
