#include "configmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
}

ConfigManager::~ConfigManager()
{
    if (m_settings) {
        m_settings->sync();
        delete m_settings;
    }
}

void ConfigManager::loadConfig(const QString &filePath)
{
    // 清理旧的 settings 对象，防止内存泄漏
    if (m_settings) {
        m_settings->sync();
        delete m_settings;
        m_settings = nullptr;
    }

    if (filePath.isEmpty()) {
        m_configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    } else {
        m_configPath = filePath;
    }

    // 如果配置文件不存在，先创建默认配置
    if (!QFile::exists(m_configPath)) {
        qInfo() << "[ConfigManager] 配置文件不存在，创建默认配置:" << m_configPath;
        m_settings = new QSettings(m_configPath, QSettings::IniFormat);
        initDefaults();
        m_settings->sync();
    } else {
        m_settings = new QSettings(m_configPath, QSettings::IniFormat);
    }

    qInfo() << "[ConfigManager] 配置加载完成:" << m_configPath;
}

void ConfigManager::saveConfig()
{
    if (m_settings) {
        m_settings->sync();
        qInfo() << "[ConfigManager] 配置已保存";
        emit configChanged();
    }
}

void ConfigManager::initDefaults()
{
    m_settings->beginGroup("Serial");
    m_settings->setValue("PortName", "");
    m_settings->setValue("BaudRate", 9600);
    m_settings->setValue("DataBits", 8);
    m_settings->setValue("Parity", 0);
    m_settings->setValue("StopBits", 1);
    m_settings->setValue("FlowControl", 0);
    m_settings->endGroup();

    m_settings->beginGroup("Network");
    m_settings->setValue("Protocol", "TCP");
    m_settings->setValue("ServerAddress", "127.0.0.1");
    m_settings->setValue("ServerPort", 9000);
    m_settings->setValue("AutoUpload", true);
    m_settings->setValue("UploadInterval", 5000);
    m_settings->endGroup();

    m_settings->beginGroup("Collect");
    m_settings->setValue("Interval", 2000);
    m_settings->endGroup();

    m_settings->beginGroup("Database");
    m_settings->setValue("Path", "envmonitor.db");
    m_settings->endGroup();
}

// ---- 串口配置 Getter/Setter ----

QString ConfigManager::serialPortName() const
{
    return m_settings->value("Serial/PortName", "").toString();
}

qint32 ConfigManager::serialBaudRate() const
{
    return m_settings->value("Serial/BaudRate", 9600).toInt();
}

QSerialPort::DataBits ConfigManager::serialDataBits() const
{
    return static_cast<QSerialPort::DataBits>(
        m_settings->value("Serial/DataBits", 8).toInt());
}

QSerialPort::Parity ConfigManager::serialParity() const
{
    return static_cast<QSerialPort::Parity>(
        m_settings->value("Serial/Parity", 0).toInt());
}

QSerialPort::StopBits ConfigManager::serialStopBits() const
{
    return static_cast<QSerialPort::StopBits>(
        m_settings->value("Serial/StopBits", 1).toInt());
}

QSerialPort::FlowControl ConfigManager::serialFlowControl() const
{
    return static_cast<QSerialPort::FlowControl>(
        m_settings->value("Serial/FlowControl", 0).toInt());
}

void ConfigManager::setSerialPortName(const QString &name)
{
    m_settings->setValue("Serial/PortName", name);
}

void ConfigManager::setSerialBaudRate(qint32 rate)
{
    m_settings->setValue("Serial/BaudRate", rate);
}

void ConfigManager::setSerialDataBits(QSerialPort::DataBits bits)
{
    m_settings->setValue("Serial/DataBits", static_cast<int>(bits));
}

void ConfigManager::setSerialParity(QSerialPort::Parity parity)
{
    m_settings->setValue("Serial/Parity", static_cast<int>(parity));
}

void ConfigManager::setSerialStopBits(QSerialPort::StopBits bits)
{
    m_settings->setValue("Serial/StopBits", static_cast<int>(bits));
}

void ConfigManager::setSerialFlowControl(QSerialPort::FlowControl flow)
{
    m_settings->setValue("Serial/FlowControl", static_cast<int>(flow));
}

// ---- 网络配置 Getter/Setter ----

QString ConfigManager::networkProtocol() const
{
    return m_settings->value("Network/Protocol", "TCP").toString();
}

QString ConfigManager::serverAddress() const
{
    return m_settings->value("Network/ServerAddress", "127.0.0.1").toString();
}

quint16 ConfigManager::serverPort() const
{
    return m_settings->value("Network/ServerPort", 9000).toUInt();
}

bool ConfigManager::autoUpload() const
{
    return m_settings->value("Network/AutoUpload", true).toBool();
}

int ConfigManager::uploadInterval() const
{
    return m_settings->value("Network/UploadInterval", 5000).toInt();
}

void ConfigManager::setNetworkProtocol(const QString &protocol)
{
    m_settings->setValue("Network/Protocol", protocol);
}

void ConfigManager::setServerAddress(const QString &addr)
{
    m_settings->setValue("Network/ServerAddress", addr);
}

void ConfigManager::setServerPort(quint16 port)
{
    m_settings->setValue("Network/ServerPort", port);
}

void ConfigManager::setAutoUpload(bool enabled)
{
    m_settings->setValue("Network/AutoUpload", enabled);
}

void ConfigManager::setUploadInterval(int ms)
{
    m_settings->setValue("Network/UploadInterval", ms);
}

// ---- 采集配置 ----

int ConfigManager::collectInterval() const
{
    return m_settings->value("Collect/Interval", 2000).toInt();
}

void ConfigManager::setCollectInterval(int ms)
{
    m_settings->setValue("Collect/Interval", ms);
}

// ---- 数据库 ----

QString ConfigManager::databasePath() const
{
    return m_settings->value("Database/Path", "envmonitor.db").toString();
}
