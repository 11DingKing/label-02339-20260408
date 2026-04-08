#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

/**
 * @brief 日志管理器（单例）
 * 负责将关键日志写入文件，支持日志分级
 */
class LogManager : public QObject
{
    Q_OBJECT

public:
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Error
    };
    Q_ENUM(LogLevel)

    static LogManager* instance();

    // 初始化日志文件
    bool initialize(const QString &logPath = "");
    void close();

    // 写入日志
    void log(LogLevel level, const QString &module, const QString &message);
    void debug(const QString &module, const QString &message);
    void info(const QString &module, const QString &message);
    void warning(const QString &module, const QString &message);
    void error(const QString &module, const QString &message);

    // 设置最小日志级别
    void setMinLevel(LogLevel level) { m_minLevel = level; }
    LogLevel minLevel() const { return m_minLevel; }

    // 是否同时输出到控制台
    void setConsoleOutput(bool enabled) { m_consoleOutput = enabled; }

signals:
    void logWritten(LogLevel level, const QString &module, const QString &message);

private:
    explicit LogManager(QObject *parent = nullptr);
    ~LogManager();
    
    static LogManager *s_instance;
    static QMutex s_mutex;

    QFile m_logFile;
    QTextStream m_stream;
    QMutex m_writeMutex;
    LogLevel m_minLevel = Info;
    bool m_consoleOutput = true;
    bool m_initialized = false;

    QString levelToString(LogLevel level) const;
};

#endif // LOGMANAGER_H
