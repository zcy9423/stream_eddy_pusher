#include "statuswidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QStyle>

StatusWidget::StatusWidget(QWidget *parent) : QGroupBox("实时监控", parent)
{
    // Shadow effect
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 20));
    shadow->setOffset(0, 2);
    this->setGraphicsEffect(shadow);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);

    auto createDataCol = [this](QString title, QLabel*& labelPtr, QString unit) -> QVBoxLayout* {
        QVBoxLayout *l = new QVBoxLayout();
        QLabel *lblT = new QLabel(title);
        lblT->setStyleSheet("color: #7f8c8d; font-size: 14px; font-weight: bold;");
        
        labelPtr = new QLabel("0.00", this);
        labelPtr->setAlignment(Qt::AlignCenter);
        labelPtr->setObjectName("BigNumber");
        
        QLabel *lblU = new QLabel(unit);
        lblU->setStyleSheet("color: #bdc3c7; font-size: 12px;");
        lblU->setAlignment(Qt::AlignCenter);

        l->addWidget(lblT, 0, Qt::AlignCenter);
        l->addWidget(labelPtr, 0, Qt::AlignCenter);
        l->addWidget(lblU, 0, Qt::AlignCenter);
        return l;
    };

    layout->addLayout(createDataCol("当前位置 (Position)", m_lblPos, "mm"));

    QFrame *line1 = new QFrame; line1->setFrameShape(QFrame::VLine); line1->setStyleSheet("color: #f0f0f0;");
    layout->addWidget(line1);

    layout->addLayout(createDataCol("实时速度 (Velocity)", m_lblSpeed, "mm/s"));

    QFrame *line2 = new QFrame; line2->setFrameShape(QFrame::VLine); line2->setStyleSheet("color: #f0f0f0;");
    layout->addWidget(line2);

    QVBoxLayout *stLayout = new QVBoxLayout();
    QLabel *stTitle = new QLabel("设备状态");
    stTitle->setStyleSheet("color: #7f8c8d; font-size: 14px; font-weight: bold;");
    
    m_lblStatus = new QLabel("未连接", this);
    m_lblStatus->setAlignment(Qt::AlignCenter);
    m_lblStatus->setObjectName("StatusBadge_Gray");
    m_lblStatus->setFixedSize(120, 36);

    stLayout->addWidget(stTitle, 0, Qt::AlignCenter);
    stLayout->addWidget(m_lblStatus, 0, Qt::AlignCenter);
    
    // --- 指示灯区域 ---
    QHBoxLayout *ledLayout = new QHBoxLayout();
    ledLayout->setSpacing(15);
    ledLayout->setContentsMargins(0, 10, 0, 0);

    auto createLed = [this](QString text) -> QLabel* {
        QLabel *led = new QLabel();
        led->setFixedSize(16, 16);
        led->setStyleSheet("background-color: #bdc3c7; border-radius: 8px; border: 1px solid #95a5a6;"); // 默认灰色
        led->setToolTip(text);
        return led;
    };

    m_ledLeftLimit = createLed("左限位 (L)");
    m_ledRightLimit = createLed("右限位 (R)");
    m_ledEmergency = createLed("急停/故障 (ESTOP)");
    
    // 标签
    auto createLedLabel = [](QString text) -> QLabel* {
        QLabel *l = new QLabel(text);
        l->setStyleSheet("color: #7f8c8d; font-size: 10px;");
        return l;
    };

    QVBoxLayout *l1 = new QVBoxLayout; l1->addWidget(m_ledLeftLimit, 0, Qt::AlignCenter); l1->addWidget(createLedLabel("左限"), 0, Qt::AlignCenter);
    QVBoxLayout *l2 = new QVBoxLayout; l2->addWidget(m_ledRightLimit, 0, Qt::AlignCenter); l2->addWidget(createLedLabel("右限"), 0, Qt::AlignCenter);
    QVBoxLayout *l3 = new QVBoxLayout; l3->addWidget(m_ledEmergency, 0, Qt::AlignCenter); l3->addWidget(createLedLabel("报警"), 0, Qt::AlignCenter);

    ledLayout->addLayout(l1);
    ledLayout->addLayout(l2);
    ledLayout->addLayout(l3);

    stLayout->addLayout(ledLayout);
    stLayout->addStretch(); 

    layout->addLayout(stLayout);
}

void StatusWidget::updateStatus(const MotionFeedback &fb)
{
    m_lblPos->setText(QString::number(fb.position_mm, 'f', 2));
    m_lblSpeed->setText(QString::number(fb.speed_mm_s, 'f', 1));

    // 更新指示灯
    QString redStyle = "background-color: #e74c3c; border-radius: 8px; border: 1px solid #c0392b;";
    QString greenStyle = "background-color: #2ecc71; border-radius: 8px; border: 1px solid #27ae60;";
    QString grayStyle = "background-color: #bdc3c7; border-radius: 8px; border: 1px solid #95a5a6;";

    // 限位：触发亮红灯，否则灰灯
    m_ledLeftLimit->setStyleSheet(fb.leftLimit ? redStyle : grayStyle);
    m_ledRightLimit->setStyleSheet(fb.rightLimit ? redStyle : grayStyle);
    
    // 报警：急停/过流/堵转 亮红灯
    bool isAlarm = fb.emergencyStop || fb.overCurrent || fb.stalled;
    m_ledEmergency->setStyleSheet(isAlarm ? redStyle : grayStyle);

    QString statusStr;
    QString badgeStyle;
    
    switch(fb.status) {
        case DeviceStatus::Idle: 
            statusStr = "空闲"; 
            badgeStyle = "StatusBadge_Gray"; 
            break;
        case DeviceStatus::MovingForward: 
            statusStr = "推进中 >>"; 
            badgeStyle = "StatusBadge_Green"; 
            break;
        case DeviceStatus::MovingBackward: 
            statusStr = "拉回中 <<"; 
            badgeStyle = "StatusBadge_Green"; 
            break;
        case DeviceStatus::Error: 
            statusStr = "故障"; 
            badgeStyle = "StatusBadge_Red"; 
            break;
        default: 
            statusStr = "未知"; 
            badgeStyle = "StatusBadge_Gray"; 
            break;
    }
    
    m_lblStatus->setText(statusStr);
    m_lblStatus->setObjectName(badgeStyle);
    // Force style update
    m_lblStatus->style()->unpolish(m_lblStatus);
    m_lblStatus->style()->polish(m_lblStatus);
}
