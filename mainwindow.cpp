#include "mainwindow.h"
#include "ui/taskconfigdialog.h" // 暂时保留，如果不需要可以删除
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <QListWidget>
#include <QStackedWidget>
#include <QSplitter>
#include <QMenuBar>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 1. 设置窗口基础属性
    this->setWindowTitle("智能运动控制系统 v1.0");
    this->resize(1200, 800); // 默认窗口大小
    
    // 初始化配置 (确保目录存在)
    ConfigManager::instance().ensureDataDirExists();
    
    // 2. 初始化控制器
    // 务必先初始化 Controller，因为 Logic 连接需要它
    m_controller = new DeviceController(this);
    
    // 3. 构建界面与样式
    initUI();
    applyStyles();  
    
    // 4. 连接逻辑
    initLogic();
    m_controller->init(); // 初始化控制器串口等
    
    // 5. 初始化数据库模型（日志）
    // 获取 DataManager 初始化的数据库连接
    QSqlDatabase db = QSqlDatabase::database(m_controller->dataManager()->connectionName());
    
    m_logModel = new QSqlTableModel(this, db);
    m_logModel->setTable("MotionLog");
    m_logModel->setSort(0, Qt::DescendingOrder); // 按时间倒序
    m_logModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    
    m_taskModel = new QSqlTableModel(this, db);
    m_taskModel->setTable("DetectionTask");
    m_taskModel->setSort(0, Qt::DescendingOrder); // 最新任务在前
    m_taskModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    
    // 将模型设置给 LogWidget
    m_logWidget->setModels(m_taskModel, m_logModel);
    
    // 定时刷新日志视图
    QTimer *viewTimer = new QTimer(this);
    connect(viewTimer, &QTimer::timeout, this, &MainWindow::updateLogView);
    viewTimer->start(1000);
}

MainWindow::~MainWindow()
{
}

void MainWindow::initUI()
{
    // --- 0. 菜单栏 ---
    QMenuBar *menuBar = this->menuBar();
    QMenu *fileMenu = menuBar->addMenu("文件 (&F)");
    
    QAction *actSettings = fileMenu->addAction("参数配置 (&S)...");
    connect(actSettings, &QAction::triggered, this, &MainWindow::onSettingsClicked);
    
    fileMenu->addSeparator();
    QAction *actExit = fileMenu->addAction("退出 (&X)");
    connect(actExit, &QAction::triggered, this, &QWidget::close);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 主布局：水平分割 (左侧导航 + 右侧内容)
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- 1. 左侧导航栏 ---
    m_navList = new QListWidget(this);
    m_navList->setFixedWidth(220); // 固定宽度
    m_navList->setFrameShape(QFrame::NoFrame); // 无边框
    m_navList->setObjectName("NavList");
    
    // 添加导航项
    auto addNavItem = [this](QString text, QString iconName) {
        QListWidgetItem *item = new QListWidgetItem(m_navList);
        item->setText(text);
        item->setSizeHint(QSize(0, 60)); // 设置行高
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    };

    addNavItem("   概览仪表盘", "dashboard");
    addNavItem("   任务配置管理", "tasks");
    addNavItem("   手动控制", "gamepad");
    addNavItem("   自动扫描任务", "repeat");
    addNavItem("   数据日志", "table");

    mainLayout->addWidget(m_navList);

    // --- 2. 右侧内容区域 (堆叠窗口) ---
    m_mainStack = new QStackedWidget(this);
    m_mainStack->setObjectName("MainStack");
    
    // -- Page 1: 仪表盘 (连接 + 状态) --
    QWidget *pageDashboard = new QWidget();
    QVBoxLayout *dashLayout = new QVBoxLayout(pageDashboard);
    dashLayout->setContentsMargins(40, 40, 40, 40);
    dashLayout->setSpacing(30);

    m_connWidget = new ConnectionWidget(this);
    m_statusWidget = new StatusWidget(this);
    
    dashLayout->addWidget(m_connWidget);
    dashLayout->addWidget(m_statusWidget);
    dashLayout->addStretch();
    
    m_mainStack->addWidget(pageDashboard);

    // -- Page 2: 任务配置 --
    QWidget *pageTask = new QWidget();
    QVBoxLayout *taskLayout = new QVBoxLayout(pageTask);
    taskLayout->setContentsMargins(40, 40, 40, 40);
    
    m_taskSetupWidget = new TaskSetupWidget(this);
    taskLayout->addWidget(m_taskSetupWidget);
    taskLayout->addStretch();
    
    m_mainStack->addWidget(pageTask);

    // -- Page 3: 手动控制 --
    QWidget *pageManual = new QWidget();
    QVBoxLayout *manualLayout = new QVBoxLayout(pageManual);
    manualLayout->setContentsMargins(40, 40, 40, 40);
    manualLayout->setSpacing(25);
    
    m_statusManual = new StatusWidget(this); // 嵌入的状态监控
    m_manualWidget = new ManualControlWidget(this);
    
    manualLayout->addWidget(m_statusManual);
    manualLayout->addWidget(m_manualWidget);
    manualLayout->addStretch();

    m_mainStack->addWidget(pageManual);

    // -- Page 3: 自动任务 --
    QWidget *pageAuto = new QWidget();
    QVBoxLayout *autoLayout = new QVBoxLayout(pageAuto);
    autoLayout->setContentsMargins(40, 40, 40, 40);
    autoLayout->setSpacing(25);

    m_statusAuto = new StatusWidget(this); // 嵌入的状态监控
    m_autoTaskWidget = new AutoTaskWidget(this);
    
    autoLayout->addWidget(m_statusAuto);
    autoLayout->addWidget(m_autoTaskWidget);
    autoLayout->addStretch();

    m_mainStack->addWidget(pageAuto);

    // -- Page 4: 数据日志 --
    QWidget *pageLog = new QWidget();
    QVBoxLayout *logLayout = new QVBoxLayout(pageLog);
    logLayout->setContentsMargins(40, 40, 40, 40);

    m_logWidget = new LogWidget(this);
    logLayout->addWidget(m_logWidget);

    m_mainStack->addWidget(pageLog);

    mainLayout->addWidget(m_mainStack);

    // 默认选中第一项
    m_navList->setCurrentRow(0);
}

void MainWindow::applyStyles()
{
    QString qss = R"(
        /* === 整体背景 === */
        QMainWindow {
            background-color: #f4f6f9;
        }

        /* === 侧边导航栏 === */
        QListWidget#NavList {
            background-color: #2c3e50;
            color: #ecf0f1;
            font-size: 16px;
            font-weight: bold;
            outline: 0;
            padding-top: 20px;
        }
        QListWidget#NavList::item {
            border-left: 5px solid transparent;
            padding-left: 20px;
            color: #bdc3c7;
        }
        QListWidget#NavList::item:hover {
            background-color: #34495e;
            color: white;
        }
        QListWidget#NavList::item:selected {
            background-color: #34495e;
            color: white;
            border-left: 5px solid #3498db; /* 选中时左侧亮条 */
        }

        /* === 右侧内容区背景 === */
        QStackedWidget#MainStack {
            background-color: #f4f6f9;
        }

        /* === 卡片样式 (保持原有) === */
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
            margin-top: 35px; 
            font-family: "Microsoft YaHei";
            font-size: 16px; 
            font-weight: bold;
            color: #34495e;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 20px;
            top: 0px; 
            padding: 0 5px;
            color: #7f8c8d;
        }

        /* === 按钮样式 === */
        QPushButton#btnConnect {
            background-color: #ffffff;
            border: 1px solid #cccccc;
            border-radius: 4px;
            color: #333333;
            font-weight: normal;
        }
        QPushButton#btnConnect:hover {
            border-color: #999999;
            background-color: #f5f5f5;
            color: #000000;
        }
        QPushButton#btnConnect:pressed {
            background-color: #e0e0e0;
        }

        QPushButton#btnAction {
            background-color: #f8f9fa; 
            border: 1px solid #d1d3d4;
            border-radius: 4px;
            color: #2c3e50;
            font-size: 16px;
            font-weight: normal;
        }
        QPushButton#btnAction:hover {
            background-color: #e9ecef;
            border-color: #adb5bd;
        }
        QPushButton#btnAction:pressed {
            background-color: #dee2e6;
        }
        QPushButton#btnAction:disabled {
            background-color: #f1f3f5;
            color: #adb5bd;
            border-color: #e9ecef;
        }

        QPushButton#btnStop {
            background-color: #343a40; 
            border: none;
            border-radius: 4px;
            color: white;
            font-size: 18px; 
            font-weight: bold;
            letter-spacing: 1px;
        }
        QPushButton#btnStop:hover {
            background-color: #23272b;
        }
        QPushButton#btnStop:pressed {
            background-color: #1d2124;
        }
        QPushButton#btnStop:disabled {
            background-color: #6c757d;
        }

        /* === 仪表盘数字 === */
        QLabel#BigNumber {
            font-family: "Segoe UI", Arial;
            font-size: 42px; 
            font-weight: normal;
            color: #333333;
        }

        /* === 状态标签徽章 === */
        QLabel#StatusBadge_Gray {
            background-color: #f0f0f0; color: #666666; border-radius: 4px; font-weight: normal; border: 1px solid #dcdcdc;
        }
        QLabel#StatusBadge_Green {
            background-color: #ffffff; color: #333333; border-radius: 4px; font-weight: bold; border: 1px solid #666666;
        }
        QLabel#StatusBadge_Red {
            background-color: #ffffff; color: #333333; border-radius: 4px; font-weight: bold; border: 1px solid #333333;
        }

        /* === 表格美化 === */
        QTableView {
            background-color: white;
            gridline-color: #eeeeee;
            selection-background-color: #e0e0e0;
            selection-color: #000000;
            font-size: 13px;
        }
        QHeaderView::section {
            background-color: #fafafa;
            padding: 8px;
            border: none;
            border-bottom: 1px solid #dcdcdc;
            color: #666666;
            font-weight: normal;
        }

        /* === 滑块美化 === */
        QSlider::groove:horizontal {
            border: 1px solid #cccccc;
            height: 4px;
            background: #f0f0f0;
            margin: 2px 0;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #ffffff;
            border: 1px solid #999999;
            width: 16px;
            height: 16px;
            margin: -7px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal:hover {
            border-color: #666666;
            background: #f9f9f9;
        }

        /* === 输入框 (SpinBox) === */
        QDoubleSpinBox, QSpinBox {
            border: 1px solid #cccccc;
            border-radius: 4px;
            padding: 5px;
            background: #ffffff;
            selection-background-color: #e0e0e0;
            selection-color: #000000;
            font-size: 14px;
            min-height: 25px;
        }
        QDoubleSpinBox:hover, QSpinBox:hover {
            border-color: #999999;
        }
        QDoubleSpinBox::up-button, QSpinBox::up-button,
        QDoubleSpinBox::down-button, QSpinBox::down-button {
            width: 20px;
            background: #f0f0f0;
            border: none;
            border-radius: 0px;
        }

        /* === 进度条 === */
        QProgressBar {
            border: 1px solid #cccccc;
            border-radius: 4px;
            background-color: #f9f9f9;
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: #666666; /* 深灰色进度 */
            border-radius: 3px;
            width: 10px; 
        }
    )";
    this->setStyleSheet(qss);
}

void MainWindow::initLogic()
{
    // --- 0. 导航栏事件 ---
    connect(m_navList, &QListWidget::itemClicked, this, &MainWindow::onNavClicked);

    // --- 1. 连接面板事件 ---
    connect(m_connWidget, &ConnectionWidget::connectClicked, this, &MainWindow::onConnectClicked);

    // --- 2. 任务配置事件 ---
    connect(m_taskSetupWidget, &TaskSetupWidget::startTaskClicked, this, [this](){
        m_controller->startNewTask(m_taskSetupWidget->operatorName(), m_taskSetupWidget->tubeId());
    });
    connect(m_taskSetupWidget, &TaskSetupWidget::endTaskClicked, this, [this](){
        m_controller->endCurrentTask();
    });

    // --- 3. 手动控制事件 ---
    connect(m_manualWidget, &ManualControlWidget::moveForwardClicked, this, &MainWindow::onForwardPressed);
    connect(m_manualWidget, &ManualControlWidget::moveBackwardClicked, this, &MainWindow::onBackwardPressed);
    connect(m_manualWidget, &ManualControlWidget::stopClicked, this, &MainWindow::onStopClicked);
    connect(m_manualWidget, &ManualControlWidget::speedChanged, this, &MainWindow::onSliderValueChanged);

    // --- 3. 自动扫描事件 ---
    TaskManager *tm = m_controller->taskManager(); // 确保 Controller 提供了 getter
    if (tm) {
        connect(m_autoTaskWidget, &AutoTaskWidget::startTaskClicked, tm, &TaskManager::startAutoScan);
        connect(m_autoTaskWidget, &AutoTaskWidget::pauseTaskClicked, tm, &TaskManager::pause);
        connect(m_autoTaskWidget, &AutoTaskWidget::resumeTaskClicked, tm, &TaskManager::resume);
        // TaskManager 反馈 -> UI
        connect(tm, &TaskManager::progressChanged, m_autoTaskWidget, &AutoTaskWidget::updateProgress);
        connect(tm, &TaskManager::stateChanged, m_autoTaskWidget, &AutoTaskWidget::updateState);
        connect(tm, &TaskManager::message, this, [](QString msg){ qDebug() << "TM Msg:" << msg; });
        connect(tm, &TaskManager::fault, this, [](QString reason){ QMessageBox::critical(nullptr, "Fault", reason); });
    }

    // --- 5. 控制器反馈事件 ---
    // 连接状态变化
    connect(m_controller, &DeviceController::connectionChanged, this, [this](bool connected){
        m_isConnected = connected;
        m_connWidget->setConnectedState(connected);
        
        // 重新计算控制权限：必须同时满足 (已连接) 和 (有任务)
        bool canControl = m_isConnected && (m_controller->currentTaskId() != -1);
        m_manualWidget->setControlsEnabled(canControl);
        m_autoTaskWidget->setEnabled(canControl);
    });
    
    // 任务状态变化
    connect(m_controller, &DeviceController::taskStateChanged, this, [this](int taskId){
        // 更新任务页状态
        m_taskSetupWidget->updateTaskState(taskId);
        
        // 只有当 (已连接 AND 有任务) 时才启用控制
        bool canControl = m_isConnected && (taskId != -1);
        m_manualWidget->setControlsEnabled(canControl);
        m_autoTaskWidget->setEnabled(canControl);
        
        if (canControl) {
             // 如果任务刚开始，可能想自动跳到手动页或保持现状？
             // 这里保持现状，让用户自己切
        }
    });

    // 实时状态更新
    connect(m_controller, &DeviceController::deviceStateUpdated, this, &MainWindow::updateStatusDisplay);
    
    // 错误处理
    connect(m_controller, &DeviceController::errorMessage, this, [this](QString msg){
        QMessageBox::critical(this, "Error", msg);
    });
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        // 配置更新后，可以在这里通知 Controller 重新加载参数
        // 例如：m_controller->reloadSettings();
        
        TaskManager *tm = m_controller->taskManager();
        if (tm) {
            auto &cfg = ConfigManager::instance();
            tm->setEdgeTimeoutMs(cfg.motionTimeout());
            // 如果有其他需要实时更新的参数，也可以在这里设置
        }
        
        QMessageBox::information(this, "提示", "参数配置已保存生效。");
    }
}

void MainWindow::onNavClicked(QListWidgetItem *item)
{
    int index = m_navList->row(item);
    if (index >= 0 && index < m_mainStack->count()) {
        m_mainStack->setCurrentIndex(index);
    }
}

void MainWindow::onConnectClicked()
{
    if (m_isConnected) {
        m_controller->requestDisconnect();
    } else {
        QString port = m_connWidget->currentPort();
        int baud = m_connWidget->currentBaud();
        m_controller->requestConnect(port, baud);
    }
}

void MainWindow::onForwardPressed()
{
    m_controller->manualMove(true, m_manualWidget->currentSpeed());
}

void MainWindow::onBackwardPressed()
{
    m_controller->manualMove(false, m_manualWidget->currentSpeed());
}

void MainWindow::onStopClicked()
{
    m_controller->stopMotion();
}

void MainWindow::onSliderValueChanged(int val)
{
    if (m_isConnected) {
        m_controller->setSpeed(val);
    }
}

void MainWindow::updateStatusDisplay(MotionFeedback fb)
{
    // 同步更新所有页面的状态监控组件
    if (m_statusWidget) m_statusWidget->updateStatus(fb);
    if (m_statusManual) m_statusManual->updateStatus(fb);
    if (m_statusAuto) m_statusAuto->updateStatus(fb);
}

void MainWindow::updateLogView()
{
    m_logWidget->refresh();
}
