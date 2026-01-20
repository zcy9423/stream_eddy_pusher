#include "mainwindow.h"
#include "ui/taskconfigdialog.h" 
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <QListWidget>
#include <QStackedWidget>
#include <QSplitter>
#include <QMenuBar>
#include <QMenu>
#include <QPainter>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 1. 设置窗口基础属性
    this->setWindowTitle("蒸发器涡流检测推拔器控制软件");
    this->resize(1600, 1000); // 宽屏长方形比例
    
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

    m_taskModel->select();
    m_taskSetupWidget->loadHistory(m_taskModel);
    
    // 定时刷新日志视图
    QTimer *viewTimer = new QTimer(this);
    connect(viewTimer, &QTimer::timeout, this, &MainWindow::updateLogView);
    viewTimer->start(1000);
}

MainWindow::~MainWindow()
{
    if (m_logModel) {
        delete m_logModel;
        m_logModel = nullptr;
    }
    if (m_taskModel) {
        delete m_taskModel;
        m_taskModel = nullptr;
    }
}

QIcon MainWindow::createIcon(const QString &text, const QColor &bg, const QColor &fg)
{
    QPixmap pix(56, 56);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    
    // 背景圆角矩形
    // p.setBrush(bg);
    // p.setPen(Qt::NoPen);
    // p.drawRoundedRect(4, 4, 48, 48, 8, 8);
    
    // 文字
    p.setPen(fg);
    QFont font("Segoe UI", 10, QFont::Bold);
    p.setFont(font);
    p.drawText(pix.rect(), Qt::AlignCenter, text);
    
    return QIcon(pix);
}

void MainWindow::initUI()
{
    // --- 隐藏默认菜单栏，使用自定义 Header ---
    this->menuBar()->hide();

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    centralWidget->setObjectName("CentralWidget");
    centralWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // 主布局：垂直 (Header + Body)
    QVBoxLayout *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // === 1. 顶部 Header (64px) ===
    QWidget *header = new QWidget(this);
    header->setFixedHeight(64);
    header->setObjectName("Header");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 0, 20, 0);
    
    // Logo 区域
    QLabel *logo = new QLabel("CONTROL PRO", this);
    logo->setObjectName("HeaderLogo");
    
    // 状态信息
    QLabel *statusLbl = new QLabel("系统状态: 就绪 (IDLE)", this);
    statusLbl->setObjectName("HeaderStatus");
    
    // 时间
    QLabel *timeLbl = new QLabel(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"), this);
    timeLbl->setObjectName("HeaderTime");
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [timeLbl](){
        timeLbl->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
    });
    timer->start(10000);

    // 右侧按钮区 (替代原菜单)
    m_btnManageUsers = new QPushButton("用户管理", this);
    m_btnManageUsers->setObjectName("HeaderBtn");
    connect(m_btnManageUsers, &QPushButton::clicked, this, &MainWindow::onManageUsersClicked);

    m_btnLogin = new QPushButton("登录/注销", this);
    m_btnLogin->setObjectName("HeaderBtn");
    connect(m_btnLogin, &QPushButton::clicked, this, &MainWindow::onLoginLogoutClicked);

    m_btnSettings = new QPushButton("系统设置", this);
    m_btnSettings->setObjectName("HeaderBtn");
    connect(m_btnSettings, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    
    QPushButton *btnExit = new QPushButton("退出", this);
    btnExit->setObjectName("HeaderBtn");
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);

    headerLayout->addWidget(logo);
    headerLayout->addSpacing(40);
    headerLayout->addWidget(statusLbl);
    headerLayout->addStretch();
    headerLayout->addWidget(timeLbl);
    headerLayout->addSpacing(20);
    headerLayout->addWidget(m_btnManageUsers);
    headerLayout->addWidget(m_btnLogin);
    headerLayout->addWidget(m_btnSettings);
    headerLayout->addWidget(btnExit);

    rootLayout->addWidget(header);

    // === 2. 下方主体 (Left Dock + Right Content) ===
    QWidget *bodyWidget = new QWidget(this);
    QHBoxLayout *bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);
    bodyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // --- 左侧 Dock 栏 (150px) ---
    QWidget *dockPanel = new QWidget(this);
    dockPanel->setFixedWidth(150); 
    dockPanel->setObjectName("DockPanel");
    
    QVBoxLayout *dockLayout = new QVBoxLayout(dockPanel);
    dockLayout->setContentsMargins(0, 20, 0, 20);
    dockLayout->setSpacing(10);

    m_navList = new QListWidget(this);
    m_navList->setFrameShape(QFrame::NoFrame);
    m_navList->setObjectName("NavList");
    m_navList->setIconSize(QSize(40, 40));
    m_navList->setFocusPolicy(Qt::NoFocus);
    
    // 添加图标导航项
    auto addNavItem = [this](QString text, QString code, QString tip) {
        QListWidgetItem *item = new QListWidgetItem(m_navList);
        // 生成简易图标
        item->setIcon(createIcon(code, Qt::transparent, QColor("#B0BEC5")));
        item->setToolTip(tip);
        item->setText(text); // 文字用于布局调整
        item->setSizeHint(QSize(140, 70)); // 调整宽度适配 Dock
        item->setTextAlignment(Qt::AlignCenter);
    };

    addNavItem("仪表盘\nDashboard", "DASH", "概览仪表盘");
    addNavItem("任务\nTasks", "TASK", "任务配置管理");
    addNavItem("手动\nManual", "MAN", "手动控制");
    addNavItem("自动\nAuto", "AUTO", "自动扫描任务");
    addNavItem("日志\nLogs", "LOG", "数据日志");

    dockLayout->addWidget(m_navList);
    bodyLayout->addWidget(dockPanel);

    // --- 右侧内容区域 ---
    m_mainStack = new QStackedWidget(this);
    m_mainStack->setObjectName("MainStack");
    
    // -- Page 1: 仪表盘 --
    QWidget *pageDashboard = new QWidget();
    QVBoxLayout *dashLayout = new QVBoxLayout(pageDashboard);
    dashLayout->setContentsMargins(30, 30, 30, 30);
    dashLayout->setSpacing(20);
    m_connWidget = new ConnectionWidget(this);
    m_statusWidget = new StatusWidget(this);
    dashLayout->addWidget(m_connWidget);
    dashLayout->addWidget(m_statusWidget);
    dashLayout->addStretch();
    m_mainStack->addWidget(pageDashboard);

    // -- Page 2: 任务配置 --
    QWidget *pageTask = new QWidget();
    QVBoxLayout *taskLayout = new QVBoxLayout(pageTask);
    taskLayout->setContentsMargins(30, 30, 30, 30);
    m_taskSetupWidget = new TaskSetupWidget(this);
    taskLayout->addWidget(m_taskSetupWidget);
    taskLayout->addStretch();
    m_mainStack->addWidget(pageTask);

    // -- Page 3: 手动控制 --
    QWidget *pageManual = new QWidget();
    QVBoxLayout *manualLayout = new QVBoxLayout(pageManual);
    manualLayout->setContentsMargins(30, 30, 30, 30);
    manualLayout->setSpacing(20);
    m_statusManual = new StatusWidget(this); 
    m_manualWidget = new ManualControlWidget(this);
    manualLayout->addWidget(m_statusManual);
    manualLayout->addWidget(m_manualWidget);
    manualLayout->addStretch();
    m_mainStack->addWidget(pageManual);

    // -- Page 4: 自动任务 --
    QWidget *pageAuto = new QWidget();
    QVBoxLayout *autoLayout = new QVBoxLayout(pageAuto);
    autoLayout->setContentsMargins(30, 30, 30, 30);
    autoLayout->setSpacing(20);
    m_statusAuto = new StatusWidget(this);
    m_autoTaskWidget = new AutoTaskWidget(this);
    autoLayout->addWidget(m_statusAuto);
    autoLayout->addWidget(m_autoTaskWidget);
    autoLayout->addStretch();
    m_mainStack->addWidget(pageAuto);

    // -- Page 5: 数据日志 --
    QWidget *pageLog = new QWidget();
    QVBoxLayout *logLayout = new QVBoxLayout(pageLog);
    logLayout->setContentsMargins(30, 30, 30, 30);
    m_logWidget = new LogWidget(this);
    logLayout->addWidget(m_logWidget);
    m_mainStack->addWidget(pageLog);

    bodyLayout->addWidget(m_mainStack);
    rootLayout->addWidget(bodyWidget);

    // === 3. 底部状态栏 (32px) ===
    QWidget *footer = new QWidget(this);
    footer->setFixedHeight(32);
    footer->setObjectName("Footer");
    QHBoxLayout *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 0, 20, 0);
    
    QLabel *footerInfo = new QLabel("Ready.", this);
    footerInfo->setObjectName("FooterText");
    
    m_lblUserInfo = new QLabel("未登录", this);
    m_lblUserInfo->setObjectName("FooterUser");
    
    footerLayout->addWidget(footerInfo);
    footerLayout->addStretch();
    footerLayout->addWidget(m_lblUserInfo);

    rootLayout->addWidget(footer);

    rootLayout->setStretch(0, 0);
    rootLayout->setStretch(1, 1);
    rootLayout->setStretch(2, 0);

    // 默认选中第一项
    m_navList->setCurrentRow(0);
}

void MainWindow::applyStyles()
{
    // 样式已在 main.cpp 中全局应用
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    if (auto *cw = centralWidget()) {
        if (auto *layout = cw->layout()) {
            QTimer::singleShot(0, this, [layout]() {
                layout->activate();
            });
        }
    }
}

void MainWindow::initLogic()
{
    // --- 0. 导航栏事件 ---
    connect(m_navList, &QListWidget::itemClicked, this, &MainWindow::onNavClicked);

    // --- 1. 连接面板事件 ---
    connect(m_connWidget, &ConnectionWidget::connectClicked, this, &MainWindow::onConnectClicked);
    // 新增取消连接事件
    connect(m_connWidget, &ConnectionWidget::cancelConnection, this, [this](){
        // 调用断开连接接口，该接口会中断 socket/serial 的操作
        m_controller->requestDisconnect();
    });

    // --- 2. 任务配置事件 ---
    connect(m_taskSetupWidget, &TaskSetupWidget::createTaskClicked, this, [this](QString op, QString tube){
        m_controller->startNewTask(op, tube);
    });
    connect(m_taskSetupWidget, &TaskSetupWidget::startTaskClicked, this, [this](int taskId){
        m_controller->activateTask(taskId);
        m_controller->updateTaskStatus(taskId, "starting");
        QMessageBox::information(this, "提示", "任务已激活，请前往控制页面操作");
    });
    connect(m_taskSetupWidget, &TaskSetupWidget::endTaskClicked, this, [this](int taskId){
        m_controller->updateTaskStatus(taskId, "stop");
        m_controller->endCurrentTask();
    });
    connect(m_taskSetupWidget, &TaskSetupWidget::deleteTaskClicked, this, [this](int taskId){
        const auto reply = QMessageBox::question(
            this,
            "确认删除",
            "确认删除此任务？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (reply != QMessageBox::Yes) {
            return;
        }
        if (m_controller->deleteTask(taskId)) {
            if (m_taskModel) m_taskModel->select();
            m_taskSetupWidget->loadHistory(m_taskModel);
        }
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
        connect(m_autoTaskWidget, &AutoTaskWidget::startSequenceClicked, tm, &TaskManager::startTaskSequence);
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
        
        if (!connected) {
             if (m_statusWidget) m_statusWidget->setDisconnected();
             if (m_statusManual) m_statusManual->setDisconnected();
             if (m_statusAuto) m_statusAuto->setDisconnected();
        }

        // 重新计算控制权限：必须同时满足 (已连接) 和 (有任务)
        bool canControl = m_isConnected && (m_controller->currentTaskId() != -1);
        m_manualWidget->setControlsEnabled(canControl);
        m_autoTaskWidget->setEnabled(canControl);
    });
    
    // 任务状态变化
    connect(m_controller, &DeviceController::taskStateChanged, this, [this](int taskId){
        // 更新任务页状态 (仅刷新按钮)
        // 注意：这里不再负责添加行，添加行由 taskCreated 负责
        m_taskSetupWidget->updateTaskState(taskId);
        
        // 只有当 (已连接 AND 有任务) 时才启用控制
        bool canControl = m_isConnected && (taskId != -1);
        m_manualWidget->setControlsEnabled(canControl);
        m_autoTaskWidget->setEnabled(canControl);
    });
    
    // 任务创建通知
    connect(m_controller, &DeviceController::taskCreated, this, [this](int taskId, QString op, QString tube){
        if (m_taskModel) m_taskModel->select();
        if (m_taskSetupWidget) {
            m_taskSetupWidget->loadHistory(m_taskModel);
            m_taskSetupWidget->updateTaskState(taskId);
        }
    });

    // 实时状态更新
    connect(m_controller, &DeviceController::deviceStateUpdated, this, &MainWindow::updateStatusDisplay);
    
    // 错误处理
    connect(m_controller, &DeviceController::errorMessage, this, [this](QString msg){
        QMessageBox::critical(this, "Error", msg);
        
        // 如果在连接过程中出错（按钮处于禁用状态且尚未连接），需要重置按钮状态
        if (!m_isConnected && m_connWidget) {
            // 调用 setConnectedState(false) 会重置按钮文字为"连接设备"并启用按钮
            m_connWidget->setConnectedState(false);
        }
    });

    // --- 6. 用户管理 ---
    connect(&UserManager::instance(), &UserManager::userChanged, this, &MainWindow::onUserChanged);
    
    // 初始化用户信息显示 (因为 main 中已经登录过了，这里手动触发一次更新)
    onUserChanged(UserManager::instance().currentUser());
}

void MainWindow::onLoginLogoutClicked()
{
    if (UserManager::instance().currentUser().role != UserManager::Guest) {
        // 已登录 -> 注销
        UserManager::instance().logout();
        // QMessageBox::information(this, "提示", "已注销登录");
        checkLogin();
    } else {
        // 未登录 -> 登录
        checkLogin();
    }
}

void MainWindow::onUserChanged(const UserManager::User &user)
{
    if (user.role == UserManager::Guest) {
        m_lblUserInfo->setText("未登录");
        m_btnLogin->setText("登录");
        m_btnSettings->setEnabled(false);
        m_btnManageUsers->setEnabled(false);
        m_btnManageUsers->setVisible(false);
    } else {
        QString roleStr = UserManager::roleName(user.role);
        m_lblUserInfo->setText(QString("%1 (%2)").arg(user.username).arg(roleStr));
        m_btnLogin->setText("注销");
        
        // 只有管理员才能进设置
        m_btnSettings->setEnabled(user.role == UserManager::Admin);
        // 只有管理员才能管理用户
        m_btnManageUsers->setEnabled(user.role == UserManager::Admin);
        m_btnManageUsers->setVisible(user.role == UserManager::Admin);
    }
}

void MainWindow::onManageUsersClicked()
{
    UserManagementDialog dlg(this);
    dlg.exec();
}

void MainWindow::checkLogin()
{
    // 如果当前未登录，弹出登录框
    if (UserManager::instance().currentUser().role == UserManager::Guest) {
        // 隐藏主窗口，营造"回到登录界面"的感觉
        this->hide();
        
        LoginDialog dlg(nullptr); // 使用 nullptr 作为父对象，确保是独立窗口
        if (dlg.exec() == QDialog::Accepted) {
            // 登录成功
            this->showMaximized();
        } else {
            // 用户在登录界面点击取消或关闭
            // 既然必须登录才能使用，这里直接退出程序
            this->close();
        }
    }
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

void MainWindow::onConnectClicked(int type, const QString &addr, int portOrBaud)
{
    if (m_isConnected) {
        m_controller->requestDisconnect();
    } else {
        m_controller->requestConnect(type, addr, portOrBaud);
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
