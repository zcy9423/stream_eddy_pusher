#include "settingsdialog.h"
#include "../core/configmanager.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("系统参数配置");
    resize(500, 400);
    initUI();
    loadSettings();
}

void SettingsDialog::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // --- 1. 串口设置 ---
    QGroupBox *grpSerial = new QGroupBox("串口设置");
    QFormLayout *layoutSerial = new QFormLayout(grpSerial);
    
    m_comboBaud = new QComboBox();
    m_comboBaud->addItems({"9600", "19200", "38400", "57600", "115200"});
    
    layoutSerial->addRow("默认波特率:", m_comboBaud);
    mainLayout->addWidget(grpSerial);

    // --- 2. 运动保护 ---
    QGroupBox *grpMotion = new QGroupBox("运动保护参数");
    QFormLayout *layoutMotion = new QFormLayout(grpMotion);

    m_spinMaxSpeed = new QDoubleSpinBox();
    m_spinMaxSpeed->setRange(1.0, 500.0);
    m_spinMaxSpeed->setSuffix(" mm/s");

    m_spinMaxPos = new QDoubleSpinBox();
    m_spinMaxPos->setRange(10.0, 5000.0);
    m_spinMaxPos->setSuffix(" mm");

    m_spinTimeout = new QSpinBox();
    m_spinTimeout->setRange(1000, 300000); // 1s - 300s
    m_spinTimeout->setSuffix(" ms");
    m_spinTimeout->setSingleStep(1000);

    layoutMotion->addRow("最大允许速度:", m_spinMaxSpeed);
    layoutMotion->addRow("最大行程限制:", m_spinMaxPos);
    layoutMotion->addRow("运动超时阈值:", m_spinTimeout);
    mainLayout->addWidget(grpMotion);

    // --- 3. 数据存储 ---
    QGroupBox *grpData = new QGroupBox("数据存储");
    QHBoxLayout *layoutData = new QHBoxLayout(grpData);
    
    m_editDataPath = new QLineEdit();
    m_editDataPath->setReadOnly(true);
    
    m_btnBrowse = new QPushButton("...");
    m_btnBrowse->setFixedWidth(40);
    m_btnBrowse->setCursor(Qt::PointingHandCursor);
    connect(m_btnBrowse, &QPushButton::clicked, this, [this](){
        QString dir = QFileDialog::getExistingDirectory(this, "选择数据存储目录", m_editDataPath->text());
        if (!dir.isEmpty()) {
            m_editDataPath->setText(dir);
        }
    });

    layoutData->addWidget(new QLabel("路径:"));
    layoutData->addWidget(m_editDataPath);
    layoutData->addWidget(m_btnBrowse);
    mainLayout->addWidget(grpData);

    mainLayout->addStretch();

    // --- 按钮 ---
    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btnBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    mainLayout->addWidget(btnBox);
}

void SettingsDialog::loadSettings()
{
    auto &cfg = ConfigManager::instance();
    
    m_comboBaud->setCurrentText(QString::number(cfg.serialBaudRate()));
    m_spinMaxSpeed->setValue(cfg.maxSpeed());
    m_spinMaxPos->setValue(cfg.maxPosition());
    m_spinTimeout->setValue(cfg.motionTimeout());
    m_editDataPath->setText(cfg.dataStoragePath());
}

void SettingsDialog::accept()
{
    auto &cfg = ConfigManager::instance();
    
    cfg.setSerialBaudRate(m_comboBaud->currentText().toInt());
    cfg.setMaxSpeed(m_spinMaxSpeed->value());
    cfg.setMaxPosition(m_spinMaxPos->value());
    cfg.setMotionTimeout(m_spinTimeout->value());
    cfg.setDataStoragePath(m_editDataPath->text());
    
    // 确保目录存在
    cfg.ensureDataDirExists();

    QDialog::accept();
}
