#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include "../models/sensordata.h"

/**
 * @brief 串口通信管理器
 * 负责串口参数配置、数据收发、状态监测
 */
class SerialManager : public QObject
{
    Q_OBJECT

public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager();

    // 串口操作
    bool openPort(const QString &portName, qint32 baudRate,
                  QSerialPort::DataBits dataBits = QSerialPort::Data8,
                  QSerialPort::Parity parity = QSerialPort::NoParity,
                  QSerialPort::StopBits stopBits = QSerialPort::OneStop,
                  QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl);
    void closePort();
    bool isConnected() const;

    // 发送数据
    bool sendData(const QByteArray &data);

    // 获取可用串口列表
    static QStringList availablePorts();

    // 模拟数据模式（用于无硬件测试）
    void startSimulation(int intervalMs = 2000);
    void stopSimulation();
    bool isSimulating() const { return m_simulating; }

signals:
    void dataReceived(const SensorData &data);
    void rawDataReceived(const QByteArray &raw);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString &error);
    void portDisconnected();

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void onSimulationTick();

private:
    SensorData parseData(const QByteArray &raw);

    QSerialPort *m_serial = nullptr;
    QByteArray m_buffer;
    QTimer *m_simTimer = nullptr;
    bool m_simulating = false;
};

#endif // SERIALMANAGER_H
