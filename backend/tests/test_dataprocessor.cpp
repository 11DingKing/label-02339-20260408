#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include "core/dataprocessor.h"
#include "managers/databasemanager.h"
#include "managers/networkmanager.h"
#include "models/sensordata.h"

/**
 * @brief DataProcessor 单元测试
 * 覆盖：数据分发、信号、与数据库/网络的集成
 */
class TestDataProcessor : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;

private slots:
    void initTestCase()
    {
        qRegisterMetaType<SensorData>("SensorData");
        QVERIFY(m_tempDir.isValid());
    }

    // ---- 基本处理 ----
    void testProcessValidData()
    {
        DataProcessor processor;
        QSignalSpy spy(&processor, &DataProcessor::dataUpdated);

        SensorData data(25.5, 60.0, 800.0);
        processor.processIncomingData(data);

        QCOMPARE(spy.count(), 1);
        SensorData received = spy.at(0).at(0).value<SensorData>();
        QCOMPARE(received.temperature(), 25.5);
        QCOMPARE(received.humidity(), 60.0);
        QCOMPARE(received.lightIntensity(), 800.0);
    }

    void testProcessInvalidData_discarded()
    {
        DataProcessor processor;
        QSignalSpy spy(&processor, &DataProcessor::dataUpdated);

        SensorData data(200.0, 60.0, 800.0);
        processor.processIncomingData(data);

        QCOMPARE(spy.count(), 0);
    }

    // ---- latestData ----
    void testLatestData()
    {
        DataProcessor processor;

        SensorData data1(20.0, 50.0, 500.0);
        processor.processIncomingData(data1);
        QCOMPARE(processor.latestData().temperature(), 20.0);

        SensorData data2(30.0, 70.0, 1500.0);
        processor.processIncomingData(data2);
        QCOMPARE(processor.latestData().temperature(), 30.0);
    }

    // ---- 与数据库集成 ----
    void testProcessData_savesToDatabase()
    {
        DatabaseManager db;
        static int dbCounter = 0;
        QString dbPath = m_tempDir.path() + QString("/test_proc_%1.db").arg(++dbCounter);
        QVERIFY(db.initialize(dbPath));

        DataProcessor processor;
        processor.setDatabaseManager(&db);

        SensorData data(25.0, 50.0, 1000.0);
        processor.processIncomingData(data);

        QCOMPARE(db.recordCount(), 1);
    }

    void testProcessData_multipleRecords()
    {
        DatabaseManager db;
        static int dbCounter2 = 0;
        QString dbPath = m_tempDir.path() + QString("/test_proc_multi_%1.db").arg(++dbCounter2);
        QVERIFY(db.initialize(dbPath));

        DataProcessor processor;
        processor.setDatabaseManager(&db);

        for (int i = 0; i < 5; ++i) {
            SensorData data(20.0 + i, 50.0, 500.0 + i * 100);
            processor.processIncomingData(data);
        }

        QCOMPARE(db.recordCount(), 5);
    }

    // ---- 无数据库时不崩溃 ----
    void testProcessData_noDatabaseManager()
    {
        DataProcessor processor;
        QSignalSpy spy(&processor, &DataProcessor::dataUpdated);

        SensorData data(25.0, 50.0, 1000.0);
        processor.processIncomingData(data);

        QCOMPARE(spy.count(), 1);
    }

    // ---- 无网络管理器时不崩溃 ----
    void testProcessData_noNetworkManager()
    {
        DataProcessor processor;
        SensorData data(25.0, 50.0, 1000.0);
        processor.processIncomingData(data);
    }

    // ---- 连续处理大量数据 ----
    void testProcessData_stress()
    {
        DatabaseManager db;
        static int dbCounter3 = 0;
        QString dbPath = m_tempDir.path() + QString("/test_stress_%1.db").arg(++dbCounter3);
        QVERIFY(db.initialize(dbPath));

        DataProcessor processor;
        processor.setDatabaseManager(&db);
        QSignalSpy spy(&processor, &DataProcessor::dataUpdated);

        for (int i = 0; i < 100; ++i) {
            SensorData data(20.0 + (i % 30), 40.0 + (i % 60), 100.0 + i * 10);
            processor.processIncomingData(data);
        }

        QCOMPARE(spy.count(), 100);
        QCOMPARE(db.recordCount(), 100);
    }
};

QTEST_MAIN(TestDataProcessor)
#include "test_dataprocessor.moc"
