#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QSerialPort>

/**
 * @brief 配置管理器
 * 负责项目配置文件的读写，使用 QSettings + INI 格式
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager();

    // 加载/保存配置
    void loadConfig(const QString &filePath = QString());
    void saveConfig();

    // 串口配置
    QString serialPortName() const;
    qint32 serialBaudRate() const;
    QSerialPort::DataBits serialDataBits() const;
    QSerialPort::Parity serialParity() const;
    QSerialPort::StopBits serialStopBits() const;
    QSerialPort::FlowControl serialFlowControl() const;

    void setSerialPortName(const QString &name);
    void setSerialBaudRate(qint32 rate);
    void setSerialDataBits(QSerialPort::DataBits bits);
    void setSerialParity(QSerialPort::Parity parity);
    void setSerialStopBits(QSerialPort::StopBits bits);
    void setSerialFlowControl(QSerialPort::FlowControl flow);

    // 网络配置
    QString networkProtocol() const;
    QString serverAddress() const;
    quint16 serverPort() const;
    bool autoUpload() const;
    int uploadInterval() const;

    void setNetworkProtocol(const QString &protocol);
    void setServerAddress(const QString &addr);
    void setServerPort(quint16 port);
    void setAutoUpload(bool enabled);
    void setUploadInterval(int ms);

    // 采集配置
    int collectInterval() const;
    void setCollectInterval(int ms);

    // 数据库路径
    QString databasePath() const;

signals:
    void configChanged();

private:
    void initDefaults();
    QSettings *m_settings = nullptr;
    QString m_configPath;
};

#endif // CONFIGMANAGER_H
