#ifndef LOGGER_H
#define LOGGER_H

#include <QDebug>
#include <QTime>

// 简单的带有时间戳的日志宏
#define LOG_INFO qInfo() << "[" << QTime::currentTime().toString("HH:mm:ss.zzz") << "][INFO]"
#define LOG_WARN qWarning() << "[" << QTime::currentTime().toString("HH:mm:ss.zzz") << "][WARN]"
#define LOG_ERR  qCritical() << "[" << QTime::currentTime().toString("HH:mm:ss.zzz") << "][ERR ]"

#endif // LOGGER_H