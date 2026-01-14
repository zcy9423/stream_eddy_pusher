#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlTableModel>
#include <QListWidget>
#include <QStackedWidget>
#include "core/devicecontroller.h"
#include "ui/connectionwidget.h"
#include "ui/statuswidget.h"
#include "ui/manualcontrolwidget.h"
#include "ui/autotaskwidget.h"
#include "ui/logwidget.h"
#include "ui/tasksetupwidget.h"
#include "ui/settingsdialog.h"
#include "core/configmanager.h"

/**
 * @brief 主窗口类
 * 
 * 负责应用程序的用户界面展示和交互。
 * 采用侧边导航栏 (Dock 风格) + 堆叠窗口的布局结构。
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

private slots:
    // --- UI 事件槽函数 ---
    
    /**
     * @brief 导航栏点击切换页面
     */
    void onNavClicked(QListWidgetItem *item);

    /**
     * @brief 处理“连接/断开”按钮点击事件
     */
    void onConnectClicked();

    // 菜单栏事件
    void onSettingsClicked();

    void onForwardPressed();
    void onBackwardPressed();
    void onStopClicked();
    void onSliderValueChanged(int val);

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

    // --- UI 组件指针 ---
    QListWidget *m_navList;        ///< 左侧导航栏
    QStackedWidget *m_mainStack;   ///< 右侧内容区域

    ConnectionWidget *m_connWidget;
    StatusWidget *m_statusWidget;      ///< 概览页的状态监控
    
    TaskSetupWidget *m_taskSetupWidget;///< 任务配置页
    
    StatusWidget *m_statusManual;      ///< 手动控制页的状态监控
    StatusWidget *m_statusAuto;        ///< 自动任务页的状态监控
    ManualControlWidget *m_manualWidget;
    AutoTaskWidget *m_autoTaskWidget;
    LogWidget *m_logWidget;

    // --- 数据模型 ---
    QSqlTableModel *m_logModel;
    QSqlTableModel *m_taskModel;

    // --- 核心逻辑 ---
    DeviceController *m_controller;
    bool m_isConnected = false;
};

#endif // MAINWINDOW_H
