#include "statuswidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QStyle>
#include <QFrame>
#include <QDateTime>

StatusWidget::StatusWidget(QWidget *parent) : QGroupBox(parent)
{
    // 隐藏默认 GroupBox 标题和边框
    this->setTitle(""); 
    this->setStyleSheet("QGroupBox { border: none; margin-top: 0px; }");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 卡片容器
    QFrame *cardFrame = new QFrame(this);
    cardFrame->setObjectName("StatusCard");
    // 移除硬编码样式

    // 移除阴影
    // QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect; ...

    // 内部布局
    QVBoxLayout *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(20);

    // 1. 标题栏 (Header)
    QLabel *lblTitle = new QLabel("实时状态监控 (Monitor)", this);
    lblTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2C3E50;"); // 适配亮色
    
    // 状态 Badge
    m_lblStatus = new QLabel("未连接", this);
    m_lblStatus->setAlignment(Qt::AlignCenter);
    m_lblStatus->setFixedSize(80, 26);
    // 背景设为透明，避免灰色块，仅在 setStyleSheet 时动态设置背景
    m_lblStatus->setStyleSheet("background-color: transparent; color: #7F8C8D; font-size: 12px; font-weight: bold;");

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(m_lblStatus);
    cardLayout->addLayout(headerLayout);

    // 分隔线
    QFrame *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #f0f0f0;");
    cardLayout->addWidget(line);

    // 2. 内容区域：左侧数据+状态，右侧图表
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // --- 左侧：数值 + 状态列表 ---
    QVBoxLayout *leftCol = new QVBoxLayout();
    leftCol->setSpacing(30);

    // 数值展示 (Metrics)
    auto createMetric = [this](QString title, QLabel*& labelPtr, QString unit, QString color) -> QFrame* {
        QFrame *f = new QFrame;
        QVBoxLayout *vl = new QVBoxLayout(f);
        vl->setContentsMargins(0,0,0,0);
        vl->setSpacing(5);
        
        QLabel *lblT = new QLabel(title);
        lblT->setStyleSheet("color: #7F8C8D; font-size: 13px; font-weight: bold;"); // 适配亮色
        
        labelPtr = new QLabel("0.00", f);
        labelPtr->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        // color 参数已经是 bright blue/green，在亮色背景下依然可见
        labelPtr->setStyleSheet(QString("font-family: 'Segoe UI', sans-serif; font-size: 32px; font-weight: bold; color: %1;").arg(color));
        
        QHBoxLayout *valRow = new QHBoxLayout;
        valRow->setSpacing(8);
        valRow->addWidget(labelPtr);
        
        QLabel *lblU = new QLabel(unit);
        lblU->setStyleSheet("color: #95A5A6; font-size: 14px; margin-top: 10px;"); // 适配亮色
        valRow->addWidget(lblU);
        valRow->addStretch();

        vl->addWidget(lblT);
        vl->addLayout(valRow);
        return f;
    };

    leftCol->addWidget(createMetric("当前位置", m_lblPos, "mm", "#2980b9"));
    leftCol->addWidget(createMetric("实时速度", m_lblSpeed, "mm/s", "#27ae60"));
    
    leftCol->addStretch();

    // 指示灯 (LEDs)
    QVBoxLayout *ledCol = new QVBoxLayout();
    ledCol->setSpacing(8);
    auto createLedRow = [this](QString text, QLabel*& ledPtr) -> QHBoxLayout* {
        QHBoxLayout *hl = new QHBoxLayout();
        hl->setSpacing(10);
        ledPtr = new QLabel();
        ledPtr->setFixedSize(10, 10);
        ledPtr->setStyleSheet("background-color: #BDC3C7; border-radius: 5px;"); // 亮色背景下的灰色
        QLabel *lbl = new QLabel(text);
        lbl->setStyleSheet("font-size: 12px; color: #7F8C8D;"); // 适配亮色
        hl->addWidget(ledPtr);
        hl->addWidget(lbl);
        hl->addStretch();
        return hl;
    };
    ledCol->addLayout(createLedRow("左限位 (L-Limit)", m_ledLeftLimit));
    ledCol->addLayout(createLedRow("右限位 (R-Limit)", m_ledRightLimit));
    ledCol->addLayout(createLedRow("故障报警 (Alarm)", m_ledEmergency));
    
    leftCol->addLayout(ledCol);
    
    // --- 右侧：实时曲线图表 ---
    initChart();
    QChartView *chartView = new QChartView(m_chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartView->setMinimumHeight(200); // 保证最小高度

    // 组合布局
    // 左侧固定宽度，右侧自适应
    QWidget *leftContainer = new QWidget;
    leftContainer->setLayout(leftCol);
    leftContainer->setFixedWidth(200); 
    // leftContainer 默认透明，无需样式

    contentLayout->addWidget(leftContainer);
    
    // 竖线分隔
    QFrame *vLine = new QFrame;
    vLine->setFrameShape(QFrame::VLine);
    vLine->setStyleSheet("color: #E6E9ED;"); // 亮色分割线
    contentLayout->addWidget(vLine);
    
    contentLayout->addWidget(chartView, 1); // 占据剩余空间

    cardLayout->addLayout(contentLayout);
    mainLayout->addWidget(cardFrame);
    
    m_startTime = QDateTime::currentMSecsSinceEpoch();
}

void StatusWidget::initChart()
{
    m_chart = new QChart();
    m_chart->setBackgroundRoundness(0);
    m_chart->setMargins(QMargins(0,0,0,0));
    m_chart->setBackgroundVisible(false); // 透明背景，融入卡片
    
    // 序列 1: 位置 (左轴)
    m_seriesPos = new QLineSeries();
    m_seriesPos->setName("位置 (mm)");
    m_seriesPos->setColor(QColor("#2980b9")); // 蓝色
    m_seriesPos->setUseOpenGL(true); // 开启 OpenGL 加速
    
    // 序列 2: 速度 (右轴)
    m_seriesSpeed = new QLineSeries();
    m_seriesSpeed->setName("速度 (mm/s)");
    m_seriesSpeed->setColor(QColor("#27ae60")); // 绿色
    m_seriesSpeed->setUseOpenGL(true);

    m_chart->addSeries(m_seriesPos);
    m_chart->addSeries(m_seriesSpeed);

    // 坐标轴 X (时间)
    m_axisX = new QValueAxis();
    m_axisX->setRange(0, 10); // 显示 10秒窗口
    m_axisX->setLabelFormat("%.1f");
    m_axisX->setTitleText("时间 (s)");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_seriesPos->attachAxis(m_axisX);
    m_seriesSpeed->attachAxis(m_axisX);

    // 坐标轴 Y1 (位置)
    m_axisYPos = new QValueAxis();
    m_axisYPos->setRange(0, 100); // 初始范围，后续自动调整
    m_axisYPos->setTitleText("位置 (mm)");
    m_axisYPos->setLinePenColor(QColor("#2980b9"));
    m_axisYPos->setLabelsColor(QColor("#2980b9"));
    m_chart->addAxis(m_axisYPos, Qt::AlignLeft);
    m_seriesPos->attachAxis(m_axisYPos);

    // 坐标轴 Y2 (速度)
    m_axisYSpeed = new QValueAxis();
    m_axisYSpeed->setRange(-10, 10);
    m_axisYSpeed->setTitleText("速度 (mm/s)");
    m_axisYSpeed->setLinePenColor(QColor("#27ae60"));
    m_axisYSpeed->setLabelsColor(QColor("#27ae60"));
    m_chart->addAxis(m_axisYSpeed, Qt::AlignRight);
    m_seriesSpeed->attachAxis(m_axisYSpeed);
    
    m_chart->legend()->setAlignment(Qt::AlignTop);
    m_chart->legend()->setVisible(true);
}

void StatusWidget::updateStatus(const MotionFeedback &fb)
{
    // 1. 更新数值显示
    m_lblPos->setText(QString::number(fb.position_mm, 'f', 2));
    m_lblSpeed->setText(QString::number(fb.speed_mm_s, 'f', 1));

    // 2. 更新指示灯
    QString redStyle = "background-color: #e74c3c; border-radius: 5px;";
    QString grayStyle = "background-color: #dcdcdc; border-radius: 5px;";

    m_ledLeftLimit->setStyleSheet(fb.leftLimit ? redStyle : grayStyle);
    m_ledRightLimit->setStyleSheet(fb.rightLimit ? redStyle : grayStyle);
    
    bool isAlarm = fb.emergencyStop || fb.overCurrent || fb.stalled;
    m_ledEmergency->setStyleSheet(isAlarm ? redStyle : grayStyle);

    QString statusStr;
    QString badgeStyle;
    
    switch(fb.status) {
        case DeviceStatus::Idle: 
            statusStr = "空闲"; 
            badgeStyle = "background-color: #ECF0F1; color: #7F8C8D;"; 
            break;
        case DeviceStatus::MovingForward: 
            statusStr = "推进中"; 
            badgeStyle = "background-color: #E8F8F5; color: #27AE60;"; 
            break;
        case DeviceStatus::MovingBackward: 
            statusStr = "拉回中"; 
            badgeStyle = "background-color: #E8F8F5; color: #27AE60;"; 
            break;
        case DeviceStatus::Error: 
            statusStr = "故障"; 
            badgeStyle = "background-color: #FDEDEC; color: #C0392B;"; 
            break;
        default: 
            statusStr = "未知"; 
            badgeStyle = "background-color: #ECF0F1; color: #7F8C8D;"; 
            break;
    }
    
    m_lblStatus->setText(statusStr);
    m_lblStatus->setStyleSheet(QString("QLabel { %1 border-radius: 4px; font-size: 12px; font-weight: bold; }").arg(badgeStyle));

    // 3. 更新图表数据
    bool isMoving = (fb.status != DeviceStatus::Idle && fb.status != DeviceStatus::Unknown);

    if (isMoving) {
        if (!m_isRecording) {
            // 刚开始运动：重置图表
            m_isRecording = true;
            m_startTime = QDateTime::currentMSecsSinceEpoch();
            m_seriesPos->clear();
            m_seriesSpeed->clear();
            m_axisX->setRange(0, 10);
            
            // 重置 Y 轴范围 (可选)
            m_axisYPos->setRange(0, 100); 
            m_axisYSpeed->setRange(-10, 10);
        }

        double t = (QDateTime::currentMSecsSinceEpoch() - m_startTime) / 1000.0;
        m_seriesPos->append(t, fb.position_mm);
        m_seriesSpeed->append(t, fb.speed_mm_s);

        // 滚动 X 轴 (保留最近 10 秒)
        if (t > 10.0) {
            m_axisX->setRange(t - 10.0, t);
        }
        
        // 自动调整 Y 轴范围
        if (fb.position_mm > m_axisYPos->max()) m_axisYPos->setMax(fb.position_mm * 1.1);
        if (fb.speed_mm_s > m_axisYSpeed->max()) m_axisYSpeed->setMax(fb.speed_mm_s * 1.2);
        if (fb.speed_mm_s < m_axisYSpeed->min()) m_axisYSpeed->setMin(fb.speed_mm_s * 1.2);
    } 
    else {
        // 设备停止或空闲
        if (m_isRecording) {
            // 记录停止瞬间的状态，然后停止刷新
            double t = (QDateTime::currentMSecsSinceEpoch() - m_startTime) / 1000.0;
            m_seriesPos->append(t, fb.position_mm);
            m_seriesSpeed->append(t, fb.speed_mm_s);
            m_isRecording = false;
        }
    }
}

void StatusWidget::setDisconnected()
{
    m_lblStatus->setText("未连接");
    
    QString badgeStyle = "background-color: transparent; color: #7F8C8D;";
    m_lblStatus->setStyleSheet(QString("QLabel { %1 border-radius: 4px; font-size: 12px; font-weight: bold; }").arg(badgeStyle));

    m_lblPos->setText("0.00");
    m_lblSpeed->setText("0.0");

    QString grayStyle = "background-color: #BDC3C7; border-radius: 5px;";
    m_ledLeftLimit->setStyleSheet(grayStyle);
    m_ledRightLimit->setStyleSheet(grayStyle);
    m_ledEmergency->setStyleSheet(grayStyle);

    m_isRecording = false;
}
