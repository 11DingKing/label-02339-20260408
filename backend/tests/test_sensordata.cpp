#include <QtTest/QtTest>
#include "models/sensordata.h"

/**
 * @brief SensorData 模型单元测试
 * 覆盖：构造、getter/setter、解析、校验、序列化
 */
class TestSensorData : public QObject
{
    Q_OBJECT

private slots:
    // ---- 构造函数 ----
    void testDefaultConstructor()
    {
        SensorData data;
        QCOMPARE(data.id(), -1);
        QCOMPARE(data.temperature(), 0.0);
        QCOMPARE(data.humidity(), 0.0);
        QCOMPARE(data.lightIntensity(), 0.0);
        QVERIFY(data.collectTime().isValid());
    }

    void testParameterizedConstructor()
    {
        SensorData data(25.5, 60.0, 800.0);
        QCOMPARE(data.temperature(), 25.5);
        QCOMPARE(data.humidity(), 60.0);
        QCOMPARE(data.lightIntensity(), 800.0);
        QVERIFY(data.collectTime().isValid());
    }

    // ---- Getter/Setter ----
    void testSettersAndGetters()
    {
        SensorData data;
        data.setId(42);
        data.setTemperature(30.5);
        data.setHumidity(75.0);
        data.setLightIntensity(1500.0);

        QDateTime now = QDateTime::currentDateTime();
        data.setCollectTime(now);

        QCOMPARE(data.id(), 42);
        QCOMPARE(data.temperature(), 30.5);
        QCOMPARE(data.humidity(), 75.0);
        QCOMPARE(data.lightIntensity(), 1500.0);
        QCOMPARE(data.collectTime(), now);
    }

    // ---- isValid 校验 ----
    void testIsValid_normalData()
    {
        SensorData data(25.0, 50.0, 1000.0);
        QVERIFY(data.isValid());
    }

    void testIsValid_boundaryLow()
    {
        SensorData data(-50.0, 0.0, 0.0);
        QVERIFY(data.isValid());
    }

    void testIsValid_boundaryHigh()
    {
        SensorData data(100.0, 100.0, 200000.0);
        QVERIFY(data.isValid());
    }

    void testIsValid_tempTooLow()
    {
        SensorData data(-51.0, 50.0, 500.0);
        QVERIFY(!data.isValid());
    }

    void testIsValid_tempTooHigh()
    {
        SensorData data(101.0, 50.0, 500.0);
        QVERIFY(!data.isValid());
    }

    void testIsValid_humTooLow()
    {
        SensorData data(25.0, -1.0, 500.0);
        QVERIFY(!data.isValid());
    }

    void testIsValid_humTooHigh()
    {
        SensorData data(25.0, 101.0, 500.0);
        QVERIFY(!data.isValid());
    }

    void testIsValid_luxTooLow()
    {
        SensorData data(25.0, 50.0, -1.0);
        QVERIFY(!data.isValid());
    }

    void testIsValid_luxTooHigh()
    {
        SensorData data(25.0, 50.0, 200001.0);
        QVERIFY(!data.isValid());
    }

    // ---- fromRawString 解析 ----
    void testFromRawString_normalFormat()
    {
        SensorData data = SensorData::fromRawString("Temp:25.5,Hum:60.0,Lux:800");
        QCOMPARE(data.temperature(), 25.5);
        QCOMPARE(data.humidity(), 60.0);
        QCOMPARE(data.lightIntensity(), 800.0);
        QVERIFY(data.isValid());
    }

    void testFromRawString_withSpaces()
    {
        SensorData data = SensorData::fromRawString("  Temp: 25.5 , Hum: 60.0 , Lux: 800  ");
        QCOMPARE(data.temperature(), 25.5);
        QCOMPARE(data.humidity(), 60.0);
        QCOMPARE(data.lightIntensity(), 800.0);
    }

    void testFromRawString_caseInsensitive()
    {
        SensorData data = SensorData::fromRawString("TEMP:30.0,HUM:70.0,LUX:1200");
        QCOMPARE(data.temperature(), 30.0);
        QCOMPARE(data.humidity(), 70.0);
        QCOMPARE(data.lightIntensity(), 1200.0);
    }

    void testFromRawString_partialData()
    {
        SensorData data = SensorData::fromRawString("Temp:25.5");
        QCOMPARE(data.temperature(), 25.5);
        QCOMPARE(data.humidity(), 0.0);
        QCOMPARE(data.lightIntensity(), 0.0);
    }

    void testFromRawString_emptyString()
    {
        SensorData data = SensorData::fromRawString("");
        QCOMPARE(data.temperature(), 0.0);
        QCOMPARE(data.humidity(), 0.0);
        QCOMPARE(data.lightIntensity(), 0.0);
    }

    void testFromRawString_garbageInput()
    {
        SensorData data = SensorData::fromRawString("hello world garbage");
        QCOMPARE(data.temperature(), 0.0);
        QCOMPARE(data.humidity(), 0.0);
    }

    void testFromRawString_invalidValues()
    {
        SensorData data = SensorData::fromRawString("Temp:abc,Hum:xyz,Lux:!!!");
        QCOMPARE(data.temperature(), 0.0);
        QCOMPARE(data.humidity(), 0.0);
        QCOMPARE(data.lightIntensity(), 0.0);
    }

    void testFromRawString_negativeValues()
    {
        SensorData data = SensorData::fromRawString("Temp:-10.5,Hum:30.0,Lux:100");
        QCOMPARE(data.temperature(), -10.5);
        QCOMPARE(data.humidity(), 30.0);
        QCOMPARE(data.lightIntensity(), 100.0);
    }

    // ---- toString / toUploadString ----
    void testToString()
    {
        SensorData data(25.5, 60.0, 800.0);
        QString str = data.toString();
        QVERIFY(str.contains("Temp:25.5"));
        QVERIFY(str.contains("Hum:60.0"));
        QVERIFY(str.contains("Lux:800.0"));
        QVERIFY(str.contains("Time:"));
    }

    void testToUploadString()
    {
        SensorData data(25.5, 60.0, 800.0);
        QString str = data.toUploadString();
        QVERIFY(str.contains("Temp:25.5"));
        QVERIFY(str.contains("Hum:60.0"));
        QVERIFY(str.contains("Lux:800.0"));
        QVERIFY(str.contains("Time:"));
        QVERIFY(str.contains(","));
    }
};

QTEST_APPLESS_MAIN(TestSensorData)
#include "test_sensordata.moc"
