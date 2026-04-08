#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "managers/databasemanager.h"
#include "models/sensordata.h"

/**
 * @brief DatabaseManager 单元测试
 * 覆盖：初始化、增删查、边界条件、异常处理
 */
class TestDatabase : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;

    QString tempDbPath()
    {
        static int counter = 0;
        return m_tempDir.path() + QString("/test_%1.db").arg(++counter);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tempDir.isValid());
    }

    // ---- 初始化 ----
    void testInitialize_success()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(tempDbPath()));
        QVERIFY(db.isConnected());
        QCOMPARE(db.recordCount(), 0);
    }

    void testInitialize_createsTable()
    {
        DatabaseManager db;
        QString path = tempDbPath();
        QVERIFY(db.initialize(path));
        SensorData data(25.0, 50.0, 1000.0);
        QVERIFY(db.insertRecord(data));
        QCOMPARE(db.recordCount(), 1);
    }

    void testIsConnected_beforeInit()
    {
        DatabaseManager db;
        QVERIFY(!db.isConnected());
    }

    // ---- 插入 ----
    void testInsertRecord_single()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(tempDbPath()));

        SensorData data(25.5, 60.0, 800.0);
        QVERIFY(db.insertRecord(data));
        QCOMPARE(db.recordCount(), 1);
    }

    void testInsertRecord_multiple()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(tempDbPath()));

        for (int i = 0; i < 10; ++i) {
            SensorData data(20.0 + i, 50.0 + i, 500.0 + i * 100);
            QVERIFY(db.insertRecord(data));
        }
        QCOMPARE(db.recordCount(), 10);
    }

    void testInsertRecord_emitsSignal()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(tempDbPath()));

        QSignalSpy spy(&db, &DatabaseManager::recordInserted);
        SensorData data(25.0, 50.0, 1000.0);
        db.insertRecord(data);
        QCOMPARE(spy.count(), 1);
    }

    void testInsertRecord_notConnected()
    {
        DatabaseManager db;
        QSignalSpy spy(&db, &DatabaseManager::databaseError);
        SensorData data(25.0, 50.0, 1000.0);
        QVERIFY(!db.insertRecord(data));
        QCOMPARE(spy.count(), 1);
    }

    // ---- 查询 ----
    void testQueryByTimeRange_found()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(tempDbPath()));

        QDateTime now = QDateTime::currentDateTime();

        SensorData data(25.5, 60.0, 800.0);
        data.setCollectTime(now);
        QVERIFY(db.insertRecord(data));

        QDateTime from = now.addSecs(-60);
        QDateTime to = now.addSecs(60);
        QList<SensorData> results = db.queryByTimeRange(from, to);

        QCOMPARE(results.size(), 1);
        QCOMPARE(results[0].temperature(), 25.5);
        QCOMPARE(results[0].humidity(), 60.0);
        QCOMPARE(results[0].lightIntensity(), 800.0);
        QVERIFY(results[0].id() > 0);
    }

    void testQueryByTimeRange_notFound()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(tempDbPath()));

        SensorData data(25.5, 60.0, 800.0);
        QVERIFY(db.insertRecord(data));

        QDateTime from = QDateTime::currentDateTime().addDays(1);
        QDateTime to = QDateTime::currentDateTime().addDays(2);
        QList<SensorData> results = db.queryByTimeRange(from, to);

        QCOMPARE(results.size(), 0);
    }

    void testQueryByTimeRange_notConnected()
    {
        DatabaseManager db;
        QDateTime from = QDateTime::currentDateTime().addDays(-1);
        QDateTime to = QDateTime::currentDateTime();
        QList<SensorData> results = db.queryByTimeRange(from, to);
        QCOMPARE(results.size(), 0);
    }

    void testQueryLatest()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(tempDbPath()));

        for (int i = 0; i < 20; ++i) {
            SensorData data(20.0 + i, 50.0, 500.0);
            QVERIFY(db.insertRecord(data));
        }

        QList<SensorData> results = db.queryLatest(5);
        QCOMPARE(results.size(), 5);
        QVERIFY(results[0].temperature() > results[4].temperature());
    }

    void testQueryLatest_lessThanRequested()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(tempDbPath()));

        SensorData data(25.0, 50.0, 1000.0);
        QVERIFY(db.insertRecord(data));

        QList<SensorData> results = db.queryLatest(100);
        QCOMPARE(results.size(), 1);
    }

    // ---- 删除 ----
    void testDeleteByTimeRange()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(tempDbPath()));

        QDateTime now = QDateTime::currentDateTime();
        SensorData data(25.0, 50.0, 1000.0);
        data.setCollectTime(now);
        QVERIFY(db.insertRecord(data));
        QCOMPARE(db.recordCount(), 1);

        int deleted = db.deleteByTimeRange(now.addSecs(-60), now.addSecs(60));
        QCOMPARE(deleted, 1);
        QCOMPARE(db.recordCount(), 0);
    }

    void testDeleteByTimeRange_noMatch()
    {
        DatabaseManager db;
        QVERIFY(db.initialize(tempDbPath()));

        SensorData data(25.0, 50.0, 1000.0);
        QVERIFY(db.insertRecord(data));

        QDateTime future = QDateTime::currentDateTime().addDays(10);
        int deleted = db.deleteByTimeRange(future, future.addDays(1));
        QCOMPARE(deleted, 0);
        QCOMPARE(db.recordCount(), 1);
    }

    void testDeleteByTimeRange_notConnected()
    {
        DatabaseManager db;
        QDateTime from = QDateTime::currentDateTime().addDays(-1);
        QDateTime to = QDateTime::currentDateTime();
        int result = db.deleteByTimeRange(from, to);
        QCOMPARE(result, -1);
    }

    // ---- 多实例隔离 ----
    void testMultipleInstances()
    {
        DatabaseManager db1;
        DatabaseManager db2;
        QVERIFY(db1.initialize(tempDbPath()));
        QVERIFY(db2.initialize(tempDbPath()));

        SensorData data(25.0, 50.0, 1000.0);
        QVERIFY(db1.insertRecord(data));
        QVERIFY(db2.insertRecord(data));

        QCOMPARE(db1.recordCount(), 1);
        QCOMPARE(db2.recordCount(), 1);
    }
};

QTEST_APPLESS_MAIN(TestDatabase)
#include "test_database.moc"
