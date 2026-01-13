#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QTimer>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <QDebug>
#include <QInputDialog>
#include <QMouseEvent>

/**
 * @brief 构造函数
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 1. 设置窗口基础属性
    this->setMinimumSize(950, 750);
    this->setWindowTitle("智能运动控制系统");
    
    // 2. 初始化控制器
    m_controller = new DeviceController(this);
    
    // 3. 构建界面与样式
    initUI();
    applyStyles();  
    
    // 4. 连接逻辑
    initLogic();
    m_controller->init(); // 初始化控制器串口等
    
    // 5. 初始化数据库模型（日志）
    m_logModel = new QSqlTableModel(this);
    m_logModel->setTable("MotionLog");
    m_logModel->setSort(0, Qt::DescendingOrder); // 按时间倒序
    m_logModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_logView->setModel(m_logModel);
    
    // 定时刷新日志视图
    QTimer *viewTimer = new QTimer(this);
    connect(viewTimer, &QTimer::timeout, this, &MainWindow::updateLogView);
    viewTimer->start(1000);
}

MainWindow::~MainWindow()
{
}

/**
 * @brief 辅助函数：创建带有样式的卡片容器
 */
QGroupBox* MainWindow::createCard(const QString &title) {
    QGroupBox *box = new QGroupBox(title, this);
    // 阴影效果（可选，如果觉得卡顿可以注释掉）
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 20)); // 半透明黑色阴影
    shadow->setOffset(0, 2);
    box->setGraphicsEffect(shadow);
    return box;
}

/**
 * @brief 初始化 UI 布局
 */
void MainWindow::initUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 全局垂直布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(30, 30, 30, 30); // 增加页面边距，更有呼吸感
    mainLayout->setSpacing(25); // 卡片之间的间距

    // ==========================================
    // 1. 顶部栏：标题 + 连接设置 (低视觉权重)
    // ==========================================
    QHBoxLayout *topBarLayout = new QHBoxLayout();
    
    // 标题
    QLabel *lblTitle = new QLabel("运动控制仪表盘", this);
    lblTitle->setObjectName("AppTitle"); 
    
    // 端口选择
    m_portCombo = new QComboBox(this);
    m_portCombo->setMinimumWidth(160);
    m_portCombo->setFixedHeight(35);
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        m_portCombo->addItem(info.portName());
    }
    m_portCombo->addItem("COM_VIRTUAL"); // 测试用虚拟串口

    // 波特率选择
    m_baudCombo = new QComboBox(this);
    m_baudCombo->setMinimumWidth(100);
    m_baudCombo->setFixedHeight(35);
    // 添加常用波特率
    QStringList baudRates = {"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"};
    m_baudCombo->addItems(baudRates);
    m_baudCombo->setCurrentText("115200"); // 默认选中 115200

    // 连接按钮
    m_btnConnect = new QPushButton("连接设备", this);
    m_btnConnect->setCursor(Qt::PointingHandCursor);
    m_btnConnect->setObjectName("btnConnect"); 
    m_btnConnect->setFixedSize(100, 35); // 固定小尺寸
    
    topBarLayout->addWidget(lblTitle);
    topBarLayout->addStretch(); // 弹簧，把右边的控件顶过去
    topBarLayout->addWidget(new QLabel("端口:"));
    topBarLayout->addWidget(m_portCombo);

    topBarLayout->addSpacing(15);

    topBarLayout->addWidget(new QLabel("波特率:"));
    topBarLayout->addWidget(m_baudCombo);

    topBarLayout->addWidget(m_btnConnect);

    mainLayout->addLayout(topBarLayout);

    // ==========================================
    // 2. 状态监控卡片 (仪表盘风格)
    // ==========================================
    QGroupBox *grpStatus = createCard("实时监控");
    QHBoxLayout *statusLayout = new QHBoxLayout(grpStatus);
    statusLayout->setContentsMargins(40, 40, 40, 40);
    
    // Lambda: 快速创建数据列
    auto createDataCol = [this](QString title, QLabel*& labelPtr, QString unit) -> QVBoxLayout* {
        QVBoxLayout *layout = new QVBoxLayout();
        QLabel *lblT = new QLabel(title);
        lblT->setStyleSheet("color: #7f8c8d; font-size: 14px; font-weight: bold;");
        
        labelPtr = new QLabel("0.00", this);
        labelPtr->setAlignment(Qt::AlignCenter);
        labelPtr->setObjectName("BigNumber"); // 绑定大号字体样式
        
        QLabel *lblU = new QLabel(unit);
        lblU->setStyleSheet("color: #bdc3c7; font-size: 12px;");
        lblU->setAlignment(Qt::AlignCenter);

        layout->addWidget(lblT, 0, Qt::AlignCenter);
        layout->addWidget(labelPtr, 0, Qt::AlignCenter);
        layout->addWidget(lblU, 0, Qt::AlignCenter);
        return layout;
    };

    // 位置
    statusLayout->addLayout(createDataCol("当前位置 (Position)", m_lblPos, "mm"));
    
    // 分割线
    QFrame *line1 = new QFrame; line1->setFrameShape(QFrame::VLine); line1->setStyleSheet("color: #f0f0f0;");
    statusLayout->addWidget(line1);

    // 速度
    statusLayout->addLayout(createDataCol("实时速度 (Velocity)", m_lblSpeed, "mm/s"));

    // 分割线
    QFrame *line2 = new QFrame; line2->setFrameShape(QFrame::VLine); line2->setStyleSheet("color: #f0f0f0;");
    statusLayout->addWidget(line2);

    // 状态文字
    QVBoxLayout *stLayout = new QVBoxLayout();
    QLabel *stTitle = new QLabel("设备状态");
    stTitle->setStyleSheet("color: #7f8c8d; font-size: 14px; font-weight: bold;");
    
    m_lblStatus = new QLabel("未连接", this);
    m_lblStatus->setAlignment(Qt::AlignCenter);
    m_lblStatus->setObjectName("StatusBadge_Gray"); // 默认灰色
    m_lblStatus->setFixedSize(120, 36);

    stLayout->addWidget(stTitle, 0, Qt::AlignCenter);
    stLayout->addWidget(m_lblStatus, 0, Qt::AlignCenter);
    stLayout->addStretch(); 

    statusLayout->addLayout(stLayout);
    mainLayout->addWidget(grpStatus);

    // ==========================================
    // 3. 控制面板 (核心操作区)
    // ==========================================
    QGroupBox *grpControl = createCard("操作面板");
    QVBoxLayout *controlWrap = new QVBoxLayout(grpControl);
    controlWrap->setContentsMargins(30, 30, 30, 30);

    // --- 速度滑块 ---
    QHBoxLayout *sliderLayout = new QHBoxLayout();
    QLabel *iconSpeed = new QLabel("⚙️ 速度设定:");
    iconSpeed->setStyleSheet("color: #34495e; font-weight: bold;");
    
    m_speedSlider = new QSlider(Qt::Horizontal, this);
    m_speedSlider->setRange(0, 100);
    m_speedSlider->setValue(20);
    
    m_lblSpeedVal = new QLabel("20%", this);
    m_lblSpeedVal->setFixedWidth(50);
    // 增加 hover 效果提示用户这里可以点击
    m_lblSpeedVal->setStyleSheet(
        "QLabel { background: #ecf0f1; border-radius: 4px; padding: 4px; font-weight: bold; }"
        "QLabel:hover { background: #dbdfe2; border: 1px solid #bdc3c7; }" // 悬浮变色
    );
    m_lblSpeedVal->setAlignment(Qt::AlignCenter);
    
    // 设置鼠标手势为小手，并安装事件过滤器
    m_lblSpeedVal->setCursor(Qt::PointingHandCursor);
    m_lblSpeedVal->setToolTip("点击直接输入数值");
    m_lblSpeedVal->installEventFilter(this); // 关键：让 MainWindow 监听这个 Label 的事件

    sliderLayout->addWidget(iconSpeed);
    sliderLayout->addWidget(m_speedSlider);
    sliderLayout->addWidget(m_lblSpeedVal);

    // --- 按钮组 (重点优化) ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20); // 按钮间距

    // 后退 (普通权重)
    m_btnBwd = new QPushButton("◀ 后退", this);
    m_btnBwd->setCursor(Qt::PointingHandCursor);
    m_btnBwd->setObjectName("btnAction"); 
    m_btnBwd->setMinimumHeight(50); // 高度适中

    // 停止 (最高权重：最大、最醒目)
    m_btnStop = new QPushButton("紧急停止", this);
    m_btnStop->setCursor(Qt::PointingHandCursor);
    m_btnStop->setObjectName("btnStop"); 
    m_btnStop->setMinimumHeight(60); // 比其他按钮更高

    // 前进 (普通权重)
    m_btnFwd = new QPushButton("前进 ▶", this);
    m_btnFwd->setCursor(Qt::PointingHandCursor);
    m_btnFwd->setObjectName("btnAction");
    m_btnFwd->setMinimumHeight(50);

    // 布局比例： 1 : 2 : 1 (停止按钮占据双倍宽度)
    btnLayout->addWidget(m_btnBwd, 1);
    btnLayout->addWidget(m_btnStop, 2); 
    btnLayout->addWidget(m_btnFwd, 1);

    controlWrap->addLayout(sliderLayout);
    controlWrap->addSpacing(25); // 滑块和按钮分开一点
    controlWrap->addLayout(btnLayout);

    mainLayout->addWidget(grpControl);

    // ==========================================
    // 4. 日志区域
    // ==========================================
    QGroupBox *grpLog = createCard("系统日志");
    QVBoxLayout *logLayout = new QVBoxLayout(grpLog);
    // 给表格一些内部边距，不要贴边
    logLayout->setContentsMargins(20, 25, 20, 20); 

    m_logView = new QTableView(this);
    m_logView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logView->horizontalHeader()->setStretchLastSection(true);
    m_logView->setAlternatingRowColors(true); // 交替颜色
    m_logView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_logView->verticalHeader()->setVisible(false);
    m_logView->setShowGrid(false); 
    m_logView->setFrameShape(QFrame::NoFrame);

    logLayout->addWidget(m_logView);
    mainLayout->addWidget(grpLog, 1); // 占据剩余空间
}

/**
 * @brief 现代风格 QSS (样式表)
 */
/**
 * @brief 现代风格 QSS (样式表) - 已优化标题位置
 */
void MainWindow::applyStyles()
{
    QString qss = R"(
        /* === 整体窗口背景 === */
        QMainWindow {
            background-color: #f4f6f9; 
        }

        /* === 标题 === */
        QLabel#AppTitle {
            font-family: "Microsoft YaHei", "Segoe UI";
            font-size: 24px;
            font-weight: 800;
            color: #2c3e50;
        }

        /* === 卡片样式 (GroupBox) === */
        QGroupBox {
            background-color: white;
            border: 1px solid #e6e6e6; 
            border-radius: 10px;
            
            /* [修改点 1] 增加顶部外边距 */
            /* 原来是12px，现在改为 35px (约1cm)，给标题留出大幅向上的空间 */
            margin-top: 35px; 
            
            font-family: "Microsoft YaHei";
            font-size: 16px; /* 稍微加大字号 */
            font-weight: bold;
            color: #34495e;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            
            /* [修改点 2] 调整标题位置 */
            left: 20px;
            top: 0px; /* 让标题位于 margin 的最顶端，从而产生“向上悬浮”的效果 */
            padding: 0 5px;
            color: #7f8c8d;
        }

        /* === 1. 连接按钮 (简约白) === */
        QPushButton#btnConnect {
            background-color: #ffffff;
            border: 1px solid #dcdcdc;
            border-radius: 6px;
            color: #666;
            font-weight: bold;
        }
        QPushButton#btnConnect:hover {
            border-color: #3498db;
            color: #3498db;
        }
        QPushButton#btnConnect:pressed {
            background-color: #f0f0f0;
        }

        /* === 2. 动作按钮 (主色蓝) === */
        QPushButton#btnAction {
            background-color: #3498db; 
            border: none;
            border-radius: 8px;
            color: white;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton#btnAction:hover {
            background-color: #2980b9;
            margin-top: -2px; 
            padding-bottom: 2px;
        }
        QPushButton#btnAction:pressed {
            background-color: #1c5980;
            margin-top: 2px;
            padding-bottom: 0px;
        }
        QPushButton#btnAction:disabled {
            background-color: #bdc3c7;
        }

        /* === 3. 停止按钮 (警示红) === */
        QPushButton#btnStop {
            background-color: #e74c3c; 
            border: none;
            border-radius: 8px;
            color: white;
            font-size: 20px; 
            font-weight: 900;
            letter-spacing: 2px;
        }
        QPushButton#btnStop:hover {
            background-color: #c0392b;
        }
        QPushButton#btnStop:pressed {
            background-color: #962d22;
        }
        QPushButton#btnStop:disabled {
            background-color: #e6b0aa;
        }

        /* === 仪表盘数字 === */
        QLabel#BigNumber {
            font-family: "Segoe UI", Arial;
            font-size: 42px; 
            font-weight: bold;
            color: #2c3e50;
        }

        /* === 状态标签徽章 === */
        QLabel#StatusBadge_Gray {
            background-color: #ecf0f1; color: #95a5a6; border-radius: 18px; font-weight: bold;
        }
        QLabel#StatusBadge_Green {
            background-color: #e8f8f5; color: #2ecc71; border-radius: 18px; font-weight: bold; border: 1px solid #2ecc71;
        }
        QLabel#StatusBadge_Red {
            background-color: #fdedec; color: #e74c3c; border-radius: 18px; font-weight: bold; border: 1px solid #e74c3c;
        }

        /* === 表格美化 === */
        QTableView {
            background-color: white;
            gridline-color: #f0f0f0;
            selection-background-color: #eaf2f8;
            selection-color: #2c3e50;
            font-size: 13px;
        }
        QHeaderView::section {
            background-color: white;
            padding: 8px;
            border: none;
            border-bottom: 2px solid #3498db;
            color: #7f8c8d;
            font-weight: bold;
        }

        /* === 滑块美化 === */
        QSlider::groove:horizontal {
            border: 1px solid #bdc3c7;
            height: 6px;
            background: #ecf0f1;
            margin: 2px 0;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #3498db;
            border: 1px solid #3498db;
            width: 16px;
            height: 16px;
            margin: -6px 0;
            border-radius: 8px;
        }
    )";

    this->setStyleSheet(qss);
}

/**
 * @brief 初始化信号槽逻辑
 */
void MainWindow::initLogic()
{
    connect(m_btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_btnFwd, &QPushButton::clicked, this, &MainWindow::onForwardPressed);
    connect(m_btnBwd, &QPushButton::clicked, this, &MainWindow::onBackwardPressed);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_speedSlider, &QSlider::valueChanged, this, &MainWindow::onSliderValueChanged);

    // 连接状态改变信号
    connect(m_controller, &DeviceController::connectionChanged, this, [this](bool connected){
        m_isConnected = connected;
        
        // 更新按钮文字和样式
        if (connected) {
            m_btnConnect->setText("断开连接");
            m_btnConnect->setStyleSheet("color: #e74c3c; border: 1px solid #e74c3c; background: #fff;");
            
            m_lblStatus->setText("已连接");
            m_lblStatus->setObjectName("StatusBadge_Green"); 
        } else {
            m_btnConnect->setText("连接设备");
            m_btnConnect->setStyleSheet(""); // 恢复默认QSS

            m_btnConnect->setEnabled(true); // 强制恢复按钮可用
            
            m_lblStatus->setText("未连接");
            m_lblStatus->setObjectName("StatusBadge_Gray");
        }
        
        // 强制刷新样式以应用新的 ObjectName
        m_lblStatus->style()->unpolish(m_lblStatus);
        m_lblStatus->style()->polish(m_lblStatus);

        m_portCombo->setEnabled(!connected);
        m_baudCombo->setEnabled(!connected);

        m_btnFwd->setEnabled(connected);
        m_btnBwd->setEnabled(connected);
        m_btnStop->setEnabled(connected);
    });

    connect(m_controller, &DeviceController::deviceStateUpdated, this, &MainWindow::updateStatusDisplay);
    
    connect(m_controller, &DeviceController::errorMessage, this, [this](QString msg){
        QMessageBox::critical(this, "系统错误", msg);
    });
}

// === 具体操作槽函数 ===

void MainWindow::onConnectClicked()
{
    if (m_isConnected) {
        m_controller->requestDisconnect();
    } else {
        QString port = m_portCombo->currentText();

        // [修改] 获取用户选择的波特率并转换为整数
        int baud = m_baudCombo->currentText().toInt();
        
        m_controller->requestConnect(port, baud);
    }
}

void MainWindow::onForwardPressed()
{
    m_controller->manualMove(true, m_speedSlider->value());
}

void MainWindow::onBackwardPressed()
{
    m_controller->manualMove(false, m_speedSlider->value());
}

void MainWindow::onStopClicked()
{
    m_controller->stopMotion();
}

void MainWindow::onSliderValueChanged(int val)
{
    m_lblSpeedVal->setText(QString("%1%").arg(val));
    if (m_isConnected) {
        m_controller->setSpeed(val);
    }
}

/**
 * @brief 更新界面显示数据
 */
void MainWindow::updateStatusDisplay(MotionFeedback fb)
{
    // 格式化小数显示
    m_lblPos->setText(QString::number(fb.position_mm, 'f', 2));
    m_lblSpeed->setText(QString::number(fb.speed_mm_s, 'f', 1));
    
    QString statusStr;
    QString styleObjName = "StatusBadge_Green"; // 默认绿色

    // 判断设备是否正在运动
    bool isMoving = false;

    switch(fb.status) {
        case DeviceStatus::Idle: 
            statusStr = "待机中"; 
            styleObjName = "StatusBadge_Green";
            isMoving = false; // 待机不是运动
            break;
        case DeviceStatus::MovingForward: 
            statusStr = "推进中 >>"; 
            styleObjName = "StatusBadge_Green"; 
            isMoving = true;  // 标记为运动中
            break;
        case DeviceStatus::MovingBackward: 
            statusStr = "<< 回退中"; 
            styleObjName = "StatusBadge_Green"; 
            isMoving = true;  // 标记为运动中
            break;
        case DeviceStatus::Error: 
            statusStr = "异常错误"; 
            styleObjName = "StatusBadge_Red"; 
            isMoving = false; // 出错通常意味着停机，允许断开重连
            break;
        default: 
            statusStr = "未知"; 
            styleObjName = "StatusBadge_Gray"; 
            isMoving = false;
            break;
    }

    // =========================================================
    // 运动保护逻辑
    // 只有在已连接的情况下，才去控制连接按钮的禁用/启用
    // =========================================================
    if (m_isConnected) {
        // 如果正在运动，禁用按钮；如果没有运动，启用按钮
        m_btnConnect->setEnabled(!isMoving);

        // 给用户一些提示，告诉他为什么按钮灰了
        if (isMoving) {
            m_btnConnect->setText("运行中锁定"); // 
            m_btnConnect->setToolTip("为了安全，设备运行时禁止断开连接。请先停止设备。");
        } else {
            m_btnConnect->setText("断开连接");   // 恢复文字
            m_btnConnect->setToolTip("");
        }
    }
    
    // 只有状态文字变化时才重绘，避免闪烁
    if (m_lblStatus->text() != statusStr) {
        m_lblStatus->setText(statusStr);
        m_lblStatus->setObjectName(styleObjName);
        m_lblStatus->style()->unpolish(m_lblStatus);
        m_lblStatus->style()->polish(m_lblStatus);
    }
}


void MainWindow::updateLogView()
{
    if (m_logModel) {
        m_logModel->select(); // 刷新数据库视图
    }
}

/**
 * @brief 事件过滤器：处理非按钮控件的交互（如点击标签）
 */
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // 判断是否是我们的速度显示标签，且事件类型为鼠标释放（点击结束）
    if (watched == m_lblSpeedVal && event->type() == QEvent::MouseButtonRelease) {
        
        bool ok;
        // 弹出整数输入对话框
        // 参数说明：父对象, 标题, 提示文字, 默认值, 最小值, 最大值, 步长, 确认标记
        int val = QInputDialog::getInt(this, 
                                       "精确速度设定", 
                                       "请输入目标速度 (0-100):", 
                                       m_speedSlider->value(), // 默认显示当前滑块的值
                                       0, 100, 1, // 范围 0-100，步长 1
                                       &ok);
        
        // 如果用户点击了 OK (ok 为 true)
        if (ok) {
            // 直接设置滑块的值
            // 注意：这会自动触发 m_speedSlider 的 valueChanged 信号
            // 进而自动调用 onSliderValueChanged，更新标签文字并发送指令
            m_speedSlider->setValue(val); 
        }
        return true; // 事件已处理，不再向下传递
    }
    
    // 对于其他控件或事件，调用父类的默认处理
    return QMainWindow::eventFilter(watched, event);
}