cmake_minimum_required(VERSION 3.16)

set(CMAKE_PREFIX_PATH "E:/qt/6.10.1/mingw_64")

project(EddyPusher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启 Qt MOC, RCC 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF) # 我们纯代码写界面，不需要 UIC

# 查找 Qt 库 (兼容 Qt6 和 Qt5)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets SerialPort Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets SerialPort Sql)

# 源文件列表 (显式列出以符合 VS Code 浏览习惯)
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    core/devicecontroller.cpp
    core/devicecontroller.h
    core/taskmanager.cpp
    core/taskmanager.h
    communication/serialmanager.cpp
    communication/serialmanager.h
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
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Sql
)

# 如果是 Windows 且是 Release 模式，可能希望隐藏控制台，可解开下行注释
# set_property(TARGET EddyPusher PROPERTY WIN32_EXECUTABLE ON)
