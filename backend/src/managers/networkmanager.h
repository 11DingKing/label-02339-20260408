#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QTimer>
#include "../models/sensordata.h"

/**
 * @brief 网络通信管理器
 * 支持 TCP/UDP 协议，实现数据上传与自动重连
 */
class NetworkManager : public QObject
{
    Q_OBJECT

public:
    enum Protocol { TCP, UDP };
    Q_ENUM(Protocol)

    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    // 连接/断开
    void connectToServer(const QString &address, quint16 port, Protocol protocol = TCP);
    void disconnectFromServer();
    bool isConnected() const;

    // 重连控制
    void setReconnectEnabled(bool enabled);
    void resetReconnect();
    int reconnectAttempts() const { return m_reconnectAttempts; }

    // 数据上传
    bool uploadData(const SensorData &data);
    bool uploadRawData(const QByteArray &data);

    // 自动上传控制
    void setAutoUpload(bool enabled, int intervalMs = 5000);
    void setLatestData(const SensorData &data);

    // 手动触发上传
    void manualUpload();

signals:
    void connectionStatusChanged(bool connected);
    void uploadSuccess();
    void uploadFailed(const QString &error);
    void serverResponse(const QByteArray &data);
    void networkError(const QString &error);

private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpReadyRead();
    void onTcpError(QAbstractSocket::SocketError error);
    void onUdpReadyRead();
    void onAutoUploadTick();
    void onReconnectTick();

private:
    void checkUdpReachability();
    
    QTcpSocket *m_tcpSocket = nullptr;
    QUdpSocket *m_udpSocket = nullptr;
    QTimer *m_autoUploadTimer = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QTimer *m_udpHeartbeatTimer = nullptr;

    Protocol m_protocol = TCP;
    QString m_serverAddress;
    quint16 m_serverPort = 9000;
    bool m_connected = false;
    bool m_autoUpload = false;
    SensorData m_latestData;
    int m_reconnectAttempts = 0;
    bool m_reconnectEnabled = true;
    int m_udpFailCount = 0;
    static const int INITIAL_RECONNECT_INTERVAL = 3000;   // 初始重连间隔 3秒
    static const int MAX_RECONNECT_INTERVAL = 60000;      // 最大重连间隔 60秒
    static const int UDP_HEARTBEAT_INTERVAL = 10000;      // UDP 心跳间隔 10秒
    static const int UDP_MAX_FAIL_COUNT = 3;              // UDP 最大失败次数
    int m_currentReconnectInterval = INITIAL_RECONNECT_INTERVAL;
};

#endif // NETWORKMANAGER_H
