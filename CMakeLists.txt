cmake_minimum_required(VERSION 3.16)

set(CMAKE_PREFIX_PATH "E:/qt/6.10.1/mingw_64")

project(EddyPusher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启 Qt MOC, RCC 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF) # 我们纯代码写界面，不需要 UIC

# 查找 Qt 库 (强制 Qt6)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort Sql Network Charts)

# 源文件列表 (显式列出以符合 VS Code 浏览习惯)
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    ui/connectionwidget.cpp
    ui/connectionwidget.h
    ui/statuswidget.cpp
    ui/statuswidget.h
    ui/manualcontrolwidget.cpp
    ui/manualcontrolwidget.h
    ui/autotaskwidget.cpp
    ui/autotaskwidget.h
    ui/taskconfigdialog.cpp
    ui/taskconfigdialog.h
    ui/tasksetupwidget.cpp
    ui/tasksetupwidget.h
    ui/settingsdialog.cpp
    ui/settingsdialog.h
    ui/logindialog.cpp
    ui/logindialog.h
    ui/usermanagementdialog.cpp
    ui/usermanagementdialog.h
    ui/logwidget.cpp
    ui/logwidget.h
    core/devicecontroller.cpp
    core/devicecontroller.h
    core/taskmanager.cpp
    core/taskmanager.h
    core/configmanager.cpp
    core/configmanager.h
    core/usermanager.cpp
    core/usermanager.h
    communication/communicationmanager.cpp
    communication/communicationmanager.h
    communication/protocol.h
    data/datamanager.cpp
    data/datamanager.h
    utils/logger.h
)

# 定义可执行文件
# 注意：开发阶段不加 WIN32 属性，以便在 VS Code 终端查看 qDebug 日志
add_executable(EddyPusher ${PROJECT_SOURCES})

# 链接库
target_link_libraries(EddyPusher PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Sql
    Qt6::Network
    Qt6::Charts
)

# 如果是 Windows 且是 Release 模式，可能希望隐藏控制台，可解开下行注释
# set_property(TARGET EddyPusher PROPERTY WIN32_EXECUTABLE ON)
