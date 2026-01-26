#include "mainwindow.h"
#include <QApplication>
#include <QLoggingCategory>
#include <QTimer>
#include <QScreen>
#include "communication/protocol.h"
#include "ui/logindialog.h"

/**
 * @brief 应用程序入口点
 * 
 * 负责初始化 Qt 应用程序对象，注册自定义元类型，并启动主窗口。
 */
int main(int argc, char *argv[])
{
    // 创建 Qt 应用程序实例
    // argc 和 argv 是命令行参数
    QApplication a(argc, argv);

    QLoggingCategory::setFilterRules("qt.core.qobject.connect.warning=false");

    // 注册自定义类型以支持跨线程 Signal/Slot 机制
    // MotionFeedback 和 ControlCommand 是我们在 protocol.h 中定义的结构体
    // 在 Qt 的信号槽机制中，如果参数是自定义类型且跨线程传递，必须注册
    qRegisterMetaType<MotionFeedback>("MotionFeedback");
    qRegisterMetaType<ControlCommand>("ControlCommand");

    // 设置应用程序元数据
    a.setApplicationName("蒸发器涡流探头推拔器控制系统");
    a.setApplicationVersion("1.0.0");

    // === 全局蓝白主题 (Modern Blue & White) ===
    QString qss = R"(
        QWidget {
            background-color: #FFFFFF;
            color: #2C3E50;
            font-family: "Segoe UI", "Microsoft YaHei";
            font-size: 10pt;
        }
        /* 顶部 Header */
        QWidget#Header {
            background-color: #FFFFFF;
            border-bottom: 1px solid #E6E9ED;
        }
        QLabel#HeaderLogo { color: #3498DB; font-size: 20px; font-weight: bold; }
        QLabel#HeaderStatus { color: #27AE60; font-weight: bold; }
        QLabel#HeaderTime { color: #7F8C8D; font-family: "Consolas", monospace; }
        QPushButton#HeaderBtn {
            background-color: transparent; border: 1px solid #BDC3C7; border-radius: 4px; color: #7F8C8D; padding: 5px 15px;
        }
        QPushButton#HeaderBtn:hover { background-color: #ECF0F1; color: #3498DB; border-color: #3498DB; }

        /* 左侧 Dock */
        QWidget#DockPanel { background-color: #FFFFFF; border-right: 1px solid #E6E9ED; }
        QListWidget#NavList { background-color: transparent; border: none; outline: none; }
        QListWidget#NavList::item { color: #7F8C8D; border-left: 4px solid transparent; padding: 10px 0; margin-bottom: 5px; }
        QListWidget#NavList::item:hover { background-color: #F0F3F4; color: #3498DB; }
        QListWidget#NavList::item:selected { background-color: #EBF5FB; border-left: 4px solid #3498DB; color: #2980B9; }

        /* 内容区 */
        QStackedWidget#MainStack { background-color: #FFFFFF; }
        
        /* 卡片 */
        QGroupBox {
            background-color: #FFFFFF; border: 1px solid #E6E9ED; border-radius: 8px; margin-top: 24px; font-weight: bold; color: #34495E;
        }
        QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 5px; color: #7F8C8D; }

        /* 底部 Footer */
        QWidget#Footer { background-color: #FFFFFF; border-top: 1px solid #E6E9ED; }
        QLabel#FooterText, QLabel#FooterUser { color: #95A5A6; font-size: 12px; }

        /* 按钮 */
        QPushButton {
            background-color: #ECF0F1; border: 1px solid #BDC3C7; border-radius: 4px; color: #2C3E50; padding: 6px 12px;
        }
        QPushButton:hover { background-color: #D6DBDF; }
        QPushButton:pressed { background-color: #BDC3C7; }
        QPushButton:disabled { background-color: #F2F4F4; color: #BDC3C7; border-color: #E5E8E8; }
        
        /* 蓝色主按钮 (需要单独指定 ObjectName 或 Class) */
        QPushButton#btnAction, QPushButton#btnConnect {
            background-color: #3498DB; color: white; border: none;
        }
        QPushButton#btnAction:hover, QPushButton#btnConnect:hover { background-color: #2980B9; }
        QPushButton#btnAction:pressed, QPushButton#btnConnect:pressed { background-color: #2471A3; }
        QPushButton#btnAction:disabled, QPushButton#btnConnect:disabled { background-color: #AED6F1; }

        /* 红色危险按钮 */
        QPushButton#btnStop {
            background-color: #E74C3C; color: white; border: none;
        }
        QPushButton#btnStop:hover { background-color: #C0392B; }
        QPushButton#btnStop:pressed { background-color: #922B21; }

        /* 输入框 */
        QLineEdit, QSpinBox, QDoubleSpinBox, QDateEdit, QDateTimeEdit, QComboBox {
            background-color: #FFFFFF; border: 1px solid #BDC3C7; border-radius: 4px; color: #2C3E50; padding: 4px;
        }
        QLineEdit:focus, QSpinBox:focus, QDateEdit:focus, QDateTimeEdit:focus, QComboBox:focus { border: 1px solid #3498DB; }
        
        /* 隐藏SpinBox的上下箭头按钮 */
        QSpinBox::up-button, QDoubleSpinBox::up-button {
            width: 0px; height: 0px; border: none;
        }
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            width: 0px; height: 0px; border: none;
        }
        
        QComboBox::drop-down { border: none; width: 20px; }

        /* 表格 */
        QTableWidget, QTableView {
            background-color: #FFFFFF; gridline-color: #F0F3F4; color: #2C3E50; border: 1px solid #E6E9ED;
        }
        QHeaderView::section {
            background-color: #F8F9F9; color: #7F8C8D; border: none; padding: 6px; font-weight: bold;
        }
        QTableWidget::item:selected, QTableView::item:selected { background-color: #EBF5FB; color: #2C3E50; }
        
        /* 标签 */
        QLabel { color: #2C3E50; }
        
        /* 对话框 */
        QDialog { background-color: #FFFFFF; }
        QMessageBox { background-color: #FFFFFF; }
        
        /* 滚动条 */
        QScrollBar:vertical {
            background: #F0F3F4; width: 8px;
        }
        QScrollBar::handle:vertical {
            background: #BDC3C7; border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background: #95A5A6;
        }
    )";
    a.setStyleSheet(qss);

    // --- 登录逻辑 ---
    // 先弹出登录框，只有登录成功才显示主界面
    LoginDialog loginDlg;
    if (loginDlg.exec() != QDialog::Accepted) {
        return 0; // 用户取消登录或关闭窗口，直接退出程序
    }

    // 创建并显示主窗口
    MainWindow w;
    
    // 正常显示窗口，不自动最大化
    w.show();

    // 进入 Qt 的主事件循环
    // exec() 会阻塞直到 exit() 被调用（通常是在最后一个窗口关闭时）
    return a.exec();
}
