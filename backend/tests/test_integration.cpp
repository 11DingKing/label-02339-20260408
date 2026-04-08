#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QUdpSocket>
#include "managers/serialmanager.h"
#include "managers/networkmanager.h"
#include "managers/databasemanager.h"
#include "managers/configmanager.h"
#include "core/dataprocessor.h"
#include "models/sensordata.h"

/**
 * @brief 集成测试
 * 覆盖完整数据流：模拟串口 -> 数据处理 -> 数据库 + 网络上传
 */
class TestIntegration : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;
    static int s_dbCounter;

    QString uniqueDbPath(const QString &prefix) {
        return m_tempDir.path() + QString("/%1_%2.db").arg(prefix).arg(++s_dbCounter);
    }

private slots:
    void initTestCase()
    {
        qRegisterMetaType<SensorData>("SensorData");
        QVERIFY(m_tempDir.isValid());
    }

    /**
     * 完整数据流：SerialManager(模拟) -> DataProcessor -> DB + Network
     */
    void testFullDataPipeline()
    {
        // 1. 数据库
        DatabaseManager db;
        QVERIFY(db.initialize(uniqueDbPath("pipeline")));

        // 2. 网络（UDP 无需真实服务器）
        QUdpSocket receiver;
        QVERIFY(receiver.bind(QHostAddress::LocalHost, 0));

        NetworkManager net;
        net.connectToServer("127.0.0.1", receiver.localPort(), NetworkManager::UDP);
        QVERIFY(net.isConnected());

        // 3. 数据处理器
        DataProcessor processor;
        processor.setDatabaseManager(&db);
        processor.setNetworkManager(&net);

        // 4. 串口模拟
        SerialManager serial;
        connect(&serial, &SerialManager::dataReceived,
                &processor, &DataProcessor::processIncomingData);

        QSignalSpy dataSpy(&processor, &DataProcessor::dataUpdated);

        serial.startSimulation(100);
        QTRY_VERIFY_WITH_TIMEOUT(dataSpy.count() >= 5, 3000);
        serial.stopSimulation();

        // 验证数据库
        QVERIFY(db.recordCount() >= 5);

        QList<SensorData> records = db.queryLatest(5);
        QCOMPARE(records.size(), 5);
        for (const SensorData &r : records) {
            QVERIFY(r.isValid());
            QVERIFY(r.id() > 0);
        }

        net.disconnectFromServer();
        qInfo() << "[集成测试] 完整数据流测试通过, 记录数:" << db.recordCount();
    }

    /**
     * 配置 -> 管理器联动
     */
    void testConfigDrivenSetup()
    {
        ConfigManager cfg;
        cfg.loadConfig(m_tempDir.path() + "/integ.ini");

        QCOMPARE(cfg.serialBaudRate(), qint32(9600));
        QCOMPARE(cfg.serverAddress(), QString("127.0.0.1"));

        cfg.setCollectInterval(500);
        cfg.setServerPort(8888);
        cfg.saveConfig();

        ConfigManager cfg2;
        cfg2.loadConfig(m_tempDir.path() + "/integ.ini");
        QCOMPARE(cfg2.collectInterval(), 500);
        QCOMPARE(cfg2.serverPort(), quint16(8888));
    }

    /**
     * 数据库完整 CRUD 流程
     */
    void testDatabaseCRUD()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(uniqueDbPath("crud")));

        QDateTime baseTime = QDateTime(QDate(2026, 1, 15), QTime(10, 0, 0));

        for (int i = 0; i < 10; ++i) {
            SensorData data(20.0 + i, 50.0 + i, 500.0 + i * 100);
            data.setCollectTime(baseTime.addSecs(i * 3600));
            QVERIFY(db.insertRecord(data));
        }
        QCOMPARE(db.recordCount(), 10);

        // 查询前5条
        QList<SensorData> results = db.queryByTimeRange(
            baseTime.addSecs(-60), baseTime.addSecs(4 * 3600 + 60));
        QCOMPARE(results.size(), 5);

        // 删除前3条
        int deleted = db.deleteByTimeRange(
            baseTime.addSecs(-60), baseTime.addSecs(2 * 3600 + 60));
        QCOMPARE(deleted, 3);
        QCOMPARE(db.recordCount(), 7);

        // 最新记录
        QList<SensorData> latest = db.queryLatest(3);
        QCOMPARE(latest.size(), 3);
        QCOMPARE(latest[0].temperature(), 29.0);
    }

    /**
     * TCP 完整通信
     */
    void testTcpFullCommunication()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));

        NetworkManager net;
        net.connectToServer("127.0.0.1", server.serverPort(), NetworkManager::TCP);
        QTRY_VERIFY_WITH_TIMEOUT(net.isConnected(), 3000);

        QVERIFY(server.waitForNewConnection(1000));
        QTcpSocket *client = server.nextPendingConnection();
        QVERIFY(client);

        QSignalSpy uploadSpy(&net, &NetworkManager::uploadSuccess);
        for (int i = 0; i < 5; ++i) {
            SensorData data(20.0 + i, 50.0 + i, 500.0 + i * 100);
            QVERIFY(net.uploadData(data));
        }
        QCOMPARE(uploadSpy.count(), 5);

        QVERIFY(client->waitForReadyRead(2000));
        QByteArray allData = client->readAll();
        QVERIFY(allData.contains("Temp:"));

        delete client;
        net.disconnectFromServer();
        server.close();
    }

    /**
     * 数据往返一致性：解析 -> 存储 -> 查询 -> 序列化
     */
    void testDataRoundTrip()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(uniqueDbPath("roundtrip")));

        SensorData parsed = SensorData::fromRawString("Temp:25.5,Hum:60.3,Lux:1234");
        QVERIFY(parsed.isValid());
        QVERIFY(db.insertRecord(parsed));

        QList<SensorData> results = db.queryLatest(1);
        QCOMPARE(results.size(), 1);

        const SensorData &loaded = results[0];
        QCOMPARE(loaded.temperature(), 25.5);
        QCOMPARE(loaded.humidity(), 60.3);
        QCOMPARE(loaded.lightIntensity(), 1234.0);

        QString uploadStr = loaded.toUploadString();
        QVERIFY(uploadStr.contains("Temp:25.5"));
        QVERIFY(uploadStr.contains("Hum:60.3"));
        QVERIFY(uploadStr.contains("Lux:1234.0"));

        qInfo() << "[集成测试] 数据往返一致性验证通过";
    }
};

int TestIntegration::s_dbCounter = 0;

QTEST_MAIN(TestIntegration)
#include "test_integration.moc"
