#include "manualcontrolwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QEvent>
#include <QInputDialog>
#include <QMouseEvent>

ManualControlWidget::ManualControlWidget(QWidget *parent)
    : QGroupBox("手动控制", parent)
{
    // 默认禁用，等待连接成功且任务创建后启用
    // 放到构造函数末尾调用 setControlsEnabled(false) 以确保组件已创建
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 20));
    shadow->setOffset(0, 2);
    this->setGraphicsEffect(shadow);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QVBoxLayout *controlWrap = new QVBoxLayout(this);
    controlWrap->setContentsMargins(30, 30, 30, 30);

    QHBoxLayout *sliderLayout = new QHBoxLayout();
    QLabel *iconSpeed = new QLabel("⚙️ 速度设定:");
    iconSpeed->setStyleSheet("color: #34495e; font-weight: bold;");
    
    m_speedSlider = new QSlider(Qt::Horizontal, this);
    m_speedSlider->setRange(0, 100);
    m_speedSlider->setValue(20);
    
    m_lblSpeedVal = new QLabel("20%", this);
    m_lblSpeedVal->setFixedWidth(50);
    m_lblSpeedVal->setStyleSheet(
        "QLabel { background: #ecf0f1; border-radius: 4px; padding: 4px; font-weight: bold; }"
        "QLabel:hover { background: #dbdfe2; border: 1px solid #bdc3c7; }"
    );
    m_lblSpeedVal->setAlignment(Qt::AlignCenter);
    m_lblSpeedVal->setCursor(Qt::PointingHandCursor);
    m_lblSpeedVal->setToolTip("点击直接输入数值");
    m_lblSpeedVal->installEventFilter(this);

    sliderLayout->addWidget(iconSpeed);
    sliderLayout->addWidget(m_speedSlider);
    sliderLayout->addWidget(m_lblSpeedVal);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20);

    m_btnBwd = new QPushButton("◀ 后退", this);
    m_btnBwd->setCursor(Qt::PointingHandCursor);
    m_btnBwd->setObjectName("btnAction"); 
    m_btnBwd->setMinimumHeight(50);

    m_btnStop = new QPushButton("紧急停止", this);
    m_btnStop->setCursor(Qt::PointingHandCursor);
    m_btnStop->setObjectName("btnStop"); 
    m_btnStop->setMinimumHeight(60);

    m_btnFwd = new QPushButton("前进 ▶", this);
    m_btnFwd->setCursor(Qt::PointingHandCursor);
    m_btnFwd->setObjectName("btnAction");
    m_btnFwd->setMinimumHeight(50);

    btnLayout->addWidget(m_btnBwd, 1);
    btnLayout->addWidget(m_btnStop, 2); 
    btnLayout->addWidget(m_btnFwd, 1);

    controlWrap->addLayout(sliderLayout);
    controlWrap->addSpacing(25);
    controlWrap->addLayout(btnLayout);

    connect(m_btnFwd, &QPushButton::clicked, this, &ManualControlWidget::moveForwardClicked);
    connect(m_btnBwd, &QPushButton::clicked, this, &ManualControlWidget::moveBackwardClicked);
    connect(m_btnStop, &QPushButton::clicked, this, &ManualControlWidget::stopClicked);
    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int val){
        m_lblSpeedVal->setText(QString("%1%").arg(val));
        emit speedChanged(val);
    });

    // 默认禁用
    setControlsEnabled(false);
}

void ManualControlWidget::setControlsEnabled(bool enabled)
{
    m_btnFwd->setEnabled(enabled);
    m_btnBwd->setEnabled(enabled);
    m_btnStop->setEnabled(enabled);
}

int ManualControlWidget::currentSpeed() const
{
    return m_speedSlider->value();
}

bool ManualControlWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_lblSpeedVal && event->type() == QEvent::MouseButtonPress) {
        bool ok;
        int val = QInputDialog::getInt(this, "速度设置", "输入速度 (0-100):", 
                                     m_speedSlider->value(), 0, 100, 1, &ok);
        if (ok) {
            m_speedSlider->setValue(val);
        }
        return true;
    }
    return QGroupBox::eventFilter(obj, event);
}
