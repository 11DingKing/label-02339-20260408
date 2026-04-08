#include "logmanager.h"
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

LogManager* LogManager::s_instance = nullptr;
QMutex LogManager::s_mutex;

LogManager::LogManager(QObject *parent)
    : QObject(parent)
{
}

LogManager::~LogManager()
{
    close();
}

LogManager* LogManager::instance()
{
    if (!s_instance) {
        QMutexLocker locker(&s_mutex);
        if (!s_instance) {
            s_instance = new LogManager();
        }
    }
    return s_instance;
}

bool LogManager::initialize(const QString &logPath)
{
    QMutexLocker locker(&m_writeMutex);
    
    if (m_initialized) {
        return true;
    }

    QString path = logPath;
    if (path.isEmpty()) {
        // 默认日志路径：应用目录下的 logs 文件夹
        QString logDir = QCoreApplication::applicationDirPath() + "/logs";
        QDir().mkpath(logDir);
        
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd");
        path = logDir + "/envmonitor_" + timestamp + ".log";
    }

    m_logFile.setFileName(path);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "[LogManager] 无法打开日志文件:" << path;
        return false;
    }

    m_stream.setDevice(&m_logFile);
    m_initialized = true;

    // 写入启动标记
    m_stream << "\n========================================\n";
    m_stream << "  EnvMonitor 日志启动\n";
    m_stream << "  时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    m_stream << "========================================\n\n";
    m_stream.flush();

    qInfo() << "[LogManager] 日志文件已初始化:" << path;
    return true;
}

void LogManager::close()
{
    QMutexLocker locker(&m_writeMutex);
    
    if (m_logFile.isOpen()) {
        m_stream << "\n[" << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") 
                 << "] 日志关闭\n";
        m_stream.flush();
        m_logFile.close();
    }
    m_initialized = false;
}

void LogManager::log(LogLevel level, const QString &module, const QString &message)
{
    if (level < m_minLevel) {
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString logLine = QString("[%1] [%2] [%3] %4")
                          .arg(timestamp, levelStr, module, message);

    // 写入文件
    if (m_initialized) {
        QMutexLocker locker(&m_writeMutex);
        m_stream << logLine << "\n";
        m_stream.flush();
    }

    // 输出到控制台
    if (m_consoleOutput) {
        switch (level) {
        case Debug:
            qDebug().noquote() << logLine;
            break;
        case Info:
            qInfo().noquote() << logLine;
            break;
        case Warning:
            qWarning().noquote() << logLine;
            break;
        case Error:
            qCritical().noquote() << logLine;
            break;
        }
    }

    emit logWritten(level, module, message);
}

void LogManager::debug(const QString &module, const QString &message)
{
    log(Debug, module, message);
}

void LogManager::info(const QString &module, const QString &message)
{
    log(Info, module, message);
}

void LogManager::warning(const QString &module, const QString &message)
{
    log(Warning, module, message);
}

void LogManager::error(const QString &module, const QString &message)
{
    log(Error, module, message);
}

QString LogManager::levelToString(LogLevel level) const
{
    switch (level) {
    case Debug:   return "DEBUG";
    case Info:    return "INFO ";
    case Warning: return "WARN ";
    case Error:   return "ERROR";
    default:      return "?????";
    }
}
