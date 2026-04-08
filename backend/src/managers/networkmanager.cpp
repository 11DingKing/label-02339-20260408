#include "networkmanager.h"
#include <QDebug>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_tcpSocket(new QTcpSocket(this))
    , m_udpSocket(new QUdpSocket(this))
    , m_autoUploadTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_udpHeartbeatTimer(new QTimer(this))
{
    connect(m_tcpSocket, &QTcpSocket::connected, this, &NetworkManager::onTcpConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &NetworkManager::onTcpDisconnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &NetworkManager::onTcpReadyRead);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, &NetworkManager::onTcpError);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::onUdpReadyRead);
    connect(m_autoUploadTimer, &QTimer::timeout, this, &NetworkManager::onAutoUploadTick);
    connect(m_reconnectTimer, &QTimer::timeout, this, &NetworkManager::onReconnectTick);
    connect(m_udpHeartbeatTimer, &QTimer::timeout, this, &NetworkManager::checkUdpReachability);
}

NetworkManager::~NetworkManager()
{
    disconnectFromServer();
}

void NetworkManager::connectToServer(const QString &address, quint16 port, Protocol protocol)
{
    m_serverAddress = address;
    m_serverPort = port;
    m_protocol = protocol;
    m_reconnectAttempts = 0;
    m_currentReconnectInterval = INITIAL_RECONNECT_INTERVAL;
    m_reconnectEnabled = true;

    if (m_protocol == TCP) {
        if (m_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
            m_tcpSocket->abort();
        }
        qInfo() << "[NetworkManager] 正在连接TCP服务器:" << address << ":" << port;
        m_tcpSocket->connectToHost(address, port);
    } else {
        // UDP 模式：绑定本地端口以接收响应
        if (!m_udpSocket->bind(QHostAddress::Any, 0)) {
            qWarning() << "[NetworkManager] UDP 绑定失败:" << m_udpSocket->errorString();
        }
        // UDP 无连接，标记为就绪状态（非严格连接）
        m_connected = true;
        m_udpFailCount = 0;
        qInfo() << "[NetworkManager] UDP模式就绪, 目标:" << address << ":" << port;
        qInfo() << "[NetworkManager] 注意: UDP 为无连接协议，状态基于发送成功率判断";
        emit connectionStatusChanged(true);
        
        // 启动 UDP 心跳检测
        m_udpHeartbeatTimer->start(UDP_HEARTBEAT_INTERVAL);
    }
}

void NetworkManager::disconnectFromServer()
{
    m_reconnectEnabled = false;
    m_autoUploadTimer->stop();
    m_reconnectTimer->stop();
    m_udpHeartbeatTimer->stop();

    if (m_protocol == TCP && m_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
        m_tcpSocket->disconnectFromHost();
    }
    if (m_protocol == UDP) {
        m_udpSocket->close();
    }
    m_connected = false;
    emit connectionStatusChanged(false);
}

bool NetworkManager::isConnected() const
{
    return m_connected;
}

void NetworkManager::setReconnectEnabled(bool enabled)
{
    m_reconnectEnabled = enabled;
    if (!enabled) {
        m_reconnectTimer->stop();
    }
}

void NetworkManager::resetReconnect()
{
    m_reconnectAttempts = 0;
    m_currentReconnectInterval = INITIAL_RECONNECT_INTERVAL;
    m_reconnectEnabled = true;
    
    // 如果当前未连接，立即尝试重连
    if (!m_connected && m_protocol == TCP && !m_serverAddress.isEmpty()) {
        m_reconnectTimer->start(100); // 立即触发
    }
}

bool NetworkManager::uploadData(const SensorData &data)
{
    QByteArray payload = data.toUploadString().toUtf8() + "\n";
    return uploadRawData(payload);
}

bool NetworkManager::uploadRawData(const QByteArray &data)
{
    if (m_protocol == TCP) {
        if (!m_connected || m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
            emit uploadFailed("TCP未连接");
            return false;
        }
        qint64 written = m_tcpSocket->write(data);
        if (written == -1) {
            emit uploadFailed("TCP发送失败: " + m_tcpSocket->errorString());
            return false;
        }
        m_tcpSocket->flush();
    } else {
        qint64 written = m_udpSocket->writeDatagram(data, QHostAddress(m_serverAddress), m_serverPort);
        if (written == -1) {
            m_udpFailCount++;
            emit uploadFailed("UDP发送失败: " + m_udpSocket->errorString());
            
            // 连续失败超过阈值，标记为断开
            if (m_udpFailCount >= UDP_MAX_FAIL_COUNT && m_connected) {
                m_connected = false;
                qWarning() << "[NetworkManager] UDP 连续发送失败，标记为断开";
                emit connectionStatusChanged(false);
                emit networkError("UDP 目标不可达，请检查网络配置");
            }
            return false;
        }
        // 发送成功，重置失败计数
        m_udpFailCount = 0;
    }

    qDebug() << "[NetworkManager] 数据已上传:" << data.trimmed();
    emit uploadSuccess();
    return true;
}

void NetworkManager::setAutoUpload(bool enabled, int intervalMs)
{
    m_autoUpload = enabled;
    if (enabled) {
        m_autoUploadTimer->start(intervalMs);
        qInfo() << "[NetworkManager] 自动上传已启用, 间隔:" << intervalMs << "ms";
    } else {
        m_autoUploadTimer->stop();
        qInfo() << "[NetworkManager] 自动上传已禁用";
    }
}

void NetworkManager::setLatestData(const SensorData &data)
{
    m_latestData = data;
}

void NetworkManager::manualUpload()
{
    if (m_latestData.isValid()) {
        uploadData(m_latestData);
    } else {
        emit uploadFailed("无可上传的数据");
    }
}

// ---- TCP 回调 ----

void NetworkManager::onTcpConnected()
{
    m_connected = true;
    m_reconnectAttempts = 0;
    m_currentReconnectInterval = INITIAL_RECONNECT_INTERVAL;
    m_reconnectTimer->stop();
    qInfo() << "[NetworkManager] TCP连接成功";
    emit connectionStatusChanged(true);
}

void NetworkManager::onTcpDisconnected()
{
    m_connected = false;
    qWarning() << "[NetworkManager] TCP连接断开";
    emit connectionStatusChanged(false);

    // 启动自动重连（无限重连，带退避机制）
    if (m_reconnectEnabled && !m_reconnectTimer->isActive()) {
        m_reconnectTimer->start(m_currentReconnectInterval);
        qInfo() << "[NetworkManager] 将在" << m_currentReconnectInterval / 1000 << "秒后尝试重连...";
    }
}

void NetworkManager::onTcpReadyRead()
{
    QByteArray data = m_tcpSocket->readAll();
    qDebug() << "[NetworkManager] 收到服务器响应:" << data;
    emit serverResponse(data);
}

void NetworkManager::onTcpError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    QString errStr = m_tcpSocket->errorString();
    qWarning() << "[NetworkManager] TCP错误:" << errStr;
    emit networkError(errStr);

    if (m_reconnectEnabled && !m_reconnectTimer->isActive()) {
        m_reconnectTimer->start(m_currentReconnectInterval);
    }
}

void NetworkManager::onAutoUploadTick()
{
    if (m_latestData.isValid() && m_connected) {
        uploadData(m_latestData);
    }
}

void NetworkManager::onReconnectTick()
{
    m_reconnectAttempts++;
    qInfo() << "[NetworkManager] 重连尝试 #" << m_reconnectAttempts 
            << " (间隔:" << m_currentReconnectInterval / 1000 << "秒)";

    m_tcpSocket->abort();
    m_tcpSocket->connectToHost(m_serverAddress, m_serverPort);

    // 指数退避：每次失败后间隔翻倍，最大60秒
    m_currentReconnectInterval = qMin(m_currentReconnectInterval * 2, MAX_RECONNECT_INTERVAL);
    m_reconnectTimer->setInterval(m_currentReconnectInterval);
}

void NetworkManager::onUdpReadyRead()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        qDebug() << "[NetworkManager] UDP 收到响应:" << datagram << "来自" << sender.toString();
        
        // 收到响应，确认连接正常
        if (!m_connected) {
            m_connected = true;
            emit connectionStatusChanged(true);
        }
        m_udpFailCount = 0;
        
        emit serverResponse(datagram);
    }
}

void NetworkManager::checkUdpReachability()
{
    if (m_protocol != UDP || m_serverAddress.isEmpty()) {
        return;
    }
    
    // 发送心跳包检测可达性
    QByteArray heartbeat = "PING\n";
    qint64 written = m_udpSocket->writeDatagram(heartbeat, QHostAddress(m_serverAddress), m_serverPort);
    
    if (written == -1) {
        m_udpFailCount++;
        qDebug() << "[NetworkManager] UDP 心跳发送失败, 失败次数:" << m_udpFailCount;
        
        if (m_udpFailCount >= UDP_MAX_FAIL_COUNT && m_connected) {
            m_connected = false;
            qWarning() << "[NetworkManager] UDP 心跳超时，标记为断开";
            emit connectionStatusChanged(false);
        }
    }
}
