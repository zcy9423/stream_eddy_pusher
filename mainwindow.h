#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLCDNumber>
#include <QSlider>
#include <QTableView>
#include <QSqlTableModel>
#include <QLabel>
#include <QGroupBox>
#include <QEvent>
#include "core/devicecontroller.h"

/**
 * @brief 主窗口类
 * 
 * 负责应用程序的用户界面展示和交互。
 * 包含连接设置、状态显示、运动控制和数据日志显示四个主要区域。
 * 通过 DeviceController 与底层硬件逻辑进行通信。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // 重写事件过滤器，用于捕获 Label 的点击事件
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    // --- UI 事件槽函数 ---
    
    /**
     * @brief 处理“连接/断开”按钮点击事件
     */
    void onConnectClicked();

    /**
     * @brief 处理“前进”按钮点击事件
     */
    void onForwardPressed();

    /**
     * @brief 处理“后退”按钮点击事件
     */
    void onBackwardPressed();

    /**
     * @brief 处理“停止”按钮点击事件
     */
    void onStopClicked();

    /**
     * @brief 处理速度滑块值改变事件
     * @param val 新的滑块值 (0-100)
     */
    void onSliderValueChanged(int val);

    // --- 业务逻辑槽函数 ---

    /**
     * @brief 更新状态显示
     * @param fb 从控制器接收到的运动反馈数据
     */
    void updateStatusDisplay(MotionFeedback fb);

    /**
     * @brief 定时刷新日志视图
     * 为了性能考虑，不是每次插入数据都刷新，而是定时刷新
     */
    void updateLogView();

private:
    /**
     * @brief 初始化用户界面
     * 使用纯代码方式构建布局和控件
     */
    void initUI();

    /**
     * @brief 初始化业务逻辑
     * 连接 UI 信号与控制器槽函数，以及控制器信号与 UI 槽函数
     */
    void initLogic();

    /**
     * @brief 应用全局样式
     * 设置 QSS 样式表，美化界面
     */
    void applyStyles();

    QGroupBox* createCard(const QString &title);

    // --- UI 组件指针 ---
    
    // 1. 连接设置区
    QComboBox *m_portCombo;    ///< 串口选择下拉框
    QComboBox *m_baudCombo;    ///< 波特率选择下拉框
    QPushButton *m_btnConnect; ///< 连接/断开按钮
    
    // 2. 状态显示区
    QLabel *m_lblPos;     ///< 位置显示
    QLabel *m_lblSpeed;    ///< 速度显示
    QLabel *m_lblStatus;       ///< 文本状态显示 (如：空闲、推进中)

    // 3. 运动控制区
    QPushButton *m_btnFwd;     ///< 前进按钮
    QPushButton *m_btnBwd;     ///< 后退按钮
    QPushButton *m_btnStop;    ///< 停止按钮
    QSlider *m_speedSlider;    ///< 速度调节滑块
    QLabel *m_lblSpeedVal;     ///< 速度数值显示标签

    // 4. 数据日志区
    QTableView *m_logView;       ///< 日志表格视图
    QSqlTableModel *m_logModel;  ///< 数据库表模型

    // --- 核心逻辑 ---
    
    DeviceController *m_controller; ///< 设备控制器实例
    bool m_isConnected = false;     ///< 当前连接状态标志
};

#endif // MAINWINDOW_H