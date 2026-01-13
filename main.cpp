#include "mainwindow.h"
#include <QApplication>
#include "communication/protocol.h"

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

    // 注册自定义类型以支持跨线程 Signal/Slot 机制
    // MotionFeedback 和 ControlCommand 是我们在 protocol.h 中定义的结构体
    // 在 Qt 的信号槽机制中，如果参数是自定义类型且跨线程传递，必须注册
    qRegisterMetaType<MotionFeedback>("MotionFeedback");
    qRegisterMetaType<ControlCommand>("ControlCommand");

    // 设置应用程序元数据
    // 这些信息在某些平台上会被用到，例如在 macOS 的关于对话框中
    a.setApplicationName("蒸发器涡流探头推拔器控制系统");
    a.setApplicationVersion("1.0.0");

    // 创建并显示主窗口
    MainWindow w;
    w.resize(1024, 768); // 设置默认窗口大小
    w.show(); // 显示窗口

    // 进入 Qt 的主事件循环
    // exec() 会阻塞直到 exit() 被调用（通常是在最后一个窗口关闭时）
    return a.exec();
}