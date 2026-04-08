#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTcpServer>
#include "managers/networkmanager.h"
#include "models/sensordata.h"

/**
 * @brief NetworkManager 单元测试
 * 覆盖：TCP/UDP连接、数据上传、自动重连、信号
 */
class TestNetwork : public QObject
{
    Q_OBJECT

private slots:
    // ---- 初始状态 ----
    void testInitialState()
    {
        NetworkManager net;
        QVERIFY(!net.isConnected());
    }

    // ---- TCP 连接成功 ----
    void testTcpConnect_success()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        quint16 port = server.serverPort();

        NetworkManager net;
        QSignalSpy statusSpy(&net, &NetworkManager::connectionStatusChanged);

        net.connectToServer("127.0.0.1", port, NetworkManager::TCP);
        QVERIFY(statusSpy.wait(3000));

        bool foundConnected = false;
        for (const auto &args : statusSpy) {
            if (args.at(0).toBool()) {
                foundConnected = true;
                break;
            }
        }
        QVERIFY(foundConnected);
        QVERIFY(net.isConnected());

        net.disconnectFromServer();
        server.close();
    }

    // ---- TCP 连接失败 ----
    void testTcpConnect_failure()
    {
        NetworkManager net;
        QSignalSpy errorSpy(&net, &NetworkManager::networkError);

        net.connectToServer("127.0.0.1", 1, NetworkManager::TCP);
        QVERIFY(errorSpy.wait(5000));
        QVERIFY(!net.isConnected());

        net.disconnectFromServer();
    }

    // ---- UDP 模式 ----
    void testUdpConnect()
    {
        NetworkManager net;
        QSignalSpy statusSpy(&net, &NetworkManager::connectionStatusChanged);

        net.connectToServer("127.0.0.1", 9999, NetworkManager::UDP);
        QVERIFY(statusSpy.count() >= 1);
        QVERIFY(net.isConnected());

        net.disconnectFromServer();
    }

    // ---- TCP 数据上传 ----
    void testTcpUploadData()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        quint16 port = server.serverPort();

        NetworkManager net;
        QSignalSpy statusSpy(&net, &NetworkManager::connectionStatusChanged);
        net.connectToServer("127.0.0.1", port, NetworkManager::TCP);
        QVERIFY(statusSpy.wait(3000));

        QVERIFY(server.waitForNewConnection(1000));
        QTcpSocket *client = server.nextPendingConnection();
        QVERIFY(client != nullptr);

        QSignalSpy uploadSpy(&net, &NetworkManager::uploadSuccess);
        SensorData data(25.5, 60.0, 800.0);
        QVERIFY(net.uploadData(data));
        QCOMPARE(uploadSpy.count(), 1);

        QVERIFY(client->waitForReadyRead(1000));
        QByteArray received = client->readAll();
        QVERIFY(received.contains("Temp:25.5"));
        QVERIFY(received.contains("Hum:60.0"));
        QVERIFY(received.contains("Lux:800.0"));

        client->close();
        net.disconnectFromServer();
        server.close();
    }

    // ---- 上传失败（未连接） ----
    void testUploadFailed_notConnected()
    {
        NetworkManager net;
        QSignalSpy failSpy(&net, &NetworkManager::uploadFailed);

        SensorData data(25.0, 50.0, 1000.0);
        QVERIFY(!net.uploadData(data));
        QCOMPARE(failSpy.count(), 1);
    }

    // ---- 手动上传 ----
    void testManualUpload_noData()
    {
        NetworkManager net;
        QSignalSpy failSpy(&net, &NetworkManager::uploadFailed);

        net.manualUpload();
        QVERIFY(failSpy.count() >= 1);
    }

    // ---- 自动上传控制 ----
    void testAutoUpload_toggle()
    {
        NetworkManager net;
        net.setAutoUpload(true, 1000);
        net.setAutoUpload(false);
        net.setAutoUpload(true, 2000);
        net.disconnectFromServer();
    }

    // ---- setLatestData ----
    void testSetLatestData()
    {
        NetworkManager net;
        SensorData data(30.0, 70.0, 1500.0);
        net.setLatestData(data);
    }

    // ---- 断开连接 ----
    void testDisconnect()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));

        NetworkManager net;
        QSignalSpy statusSpy(&net, &NetworkManager::connectionStatusChanged);
        net.connectToServer("127.0.0.1", server.serverPort(), NetworkManager::TCP);
        QVERIFY(statusSpy.wait(3000));

        net.disconnectFromServer();

        bool foundDisconnected = false;
        for (const auto &args : statusSpy) {
            if (!args.at(0).toBool()) {
                foundDisconnected = true;
                break;
            }
        }
        QVERIFY(foundDisconnected);
        QVERIFY(!net.isConnected());

        server.close();
    }

    // ---- UDP 上传 ----
    void testUdpUploadData()
    {
        QUdpSocket receiver;
        QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));
        quint16 port = receiver.localPort();

        NetworkManager net;
        net.connectToServer("127.0.0.1", port, NetworkManager::UDP);
        QVERIFY(net.isConnected());

        QSignalSpy uploadSpy(&net, &NetworkManager::uploadSuccess);
        SensorData data(22.0, 55.0, 600.0);
        QVERIFY(net.uploadData(data));
        QCOMPARE(uploadSpy.count(), 1);

        QVERIFY(receiver.waitForReadyRead(1000));
        QByteArray datagram;
        datagram.resize(receiver.pendingDatagramSize());
        receiver.readDatagram(datagram.data(), datagram.size());
        QVERIFY(datagram.contains("Temp:22.0"));

        net.disconnectFromServer();
    }
};

QTEST_MAIN(TestNetwork)
#include "test_network.moc"
