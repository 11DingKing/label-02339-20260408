#include <QtTest/QtTest>
#include <QSignalSpy>
#include "managers/serialmanager.h"
#include "models/sensordata.h"

/**
 * @brief SerialManager 单元测试
 * 覆盖：模拟模式、信号发射、数据解析、状态管理
 */
class TestSerial : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<SensorData>("SensorData");
    }

    // ---- 初始状态 ----
    void testInitialState()
    {
        SerialManager serial;
        QVERIFY(!serial.isConnected());
        QVERIFY(!serial.isSimulating());
    }

    // ---- 可用串口列表 ----
    void testAvailablePorts()
    {
        QStringList ports = SerialManager::availablePorts();
        Q_UNUSED(ports)
    }

    // ---- 模拟模式启动/停止 ----
    void testSimulation_startStop()
    {
        SerialManager serial;
        QSignalSpy statusSpy(&serial, &SerialManager::connectionStatusChanged);

        serial.startSimulation(100);
        QVERIFY(serial.isSimulating());
        QCOMPARE(statusSpy.count(), 1);
        QCOMPARE(statusSpy.last().at(0).toBool(), true);

        serial.stopSimulation();
        QVERIFY(!serial.isSimulating());
        QCOMPARE(statusSpy.count(), 2);
        QCOMPARE(statusSpy.last().at(0).toBool(), false);
    }

    // ---- 模拟模式产生数据 ----
    void testSimulation_generatesData()
    {
        SerialManager serial;
        QSignalSpy dataSpy(&serial, &SerialManager::dataReceived);
        QSignalSpy rawSpy(&serial, &SerialManager::rawDataReceived);

        serial.startSimulation(50);

        QTRY_VERIFY_WITH_TIMEOUT(dataSpy.count() >= 3, 2000);
        QVERIFY(rawSpy.count() >= 3);

        for (const auto &args : dataSpy) {
            SensorData data = args.at(0).value<SensorData>();
            QVERIFY(data.isValid());
            QVERIFY(data.temperature() >= 20.0 && data.temperature() <= 35.0);
            QVERIFY(data.humidity() >= 40.0 && data.humidity() <= 80.0);
            QVERIFY(data.lightIntensity() >= 200.0 && data.lightIntensity() <= 2000.0);
        }

        serial.stopSimulation();
    }

    // ---- 模拟模式重复启停 ----
    void testSimulation_restartWithNewInterval()
    {
        SerialManager serial;

        serial.startSimulation(200);
        QVERIFY(serial.isSimulating());

        serial.stopSimulation();
        QVERIFY(!serial.isSimulating());

        serial.startSimulation(100);
        QVERIFY(serial.isSimulating());

        serial.stopSimulation();
        QVERIFY(!serial.isSimulating());
    }

    // ---- 停止未启动的模拟不崩溃 ----
    void testSimulation_stopWithoutStart()
    {
        SerialManager serial;
        serial.stopSimulation();
        QVERIFY(!serial.isSimulating());
    }

    // ---- 打开不存在的串口 ----
    void testOpenPort_invalidPort()
    {
        SerialManager serial;
        QSignalSpy errorSpy(&serial, &SerialManager::errorOccurred);
        QSignalSpy statusSpy(&serial, &SerialManager::connectionStatusChanged);

        bool result = serial.openPort("NONEXISTENT_PORT_XYZ", 9600);
        QVERIFY(!result);
        QVERIFY(!serial.isConnected());
        QCOMPARE(errorSpy.count(), 1);
        QVERIFY(errorSpy.last().at(0).toString().contains("串口打开失败"));
    }

    // ---- 未打开时发送数据 ----
    void testSendData_notConnected()
    {
        SerialManager serial;
        QSignalSpy errorSpy(&serial, &SerialManager::errorOccurred);

        bool result = serial.sendData("test data");
        QVERIFY(!result);
        QCOMPARE(errorSpy.count(), 1);
    }

    // ---- 关闭未打开的串口不崩溃 ----
    void testClosePort_notOpen()
    {
        SerialManager serial;
        serial.closePort();
        QVERIFY(!serial.isConnected());
    }

    // ---- 析构时自动清理 ----
    void testDestructor_cleansUp()
    {
        auto *serial = new SerialManager();
        serial->startSimulation(100);
        QVERIFY(serial->isSimulating());
        delete serial;
    }
};

QTEST_MAIN(TestSerial)
#include "test_serial.moc"
