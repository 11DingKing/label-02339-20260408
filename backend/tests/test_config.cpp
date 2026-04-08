#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "managers/configmanager.h"

/**
 * @brief ConfigManager 单元测试
 * 覆盖：默认值、读写、持久化、重复加载
 */
class TestConfig : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;

    QString tempConfigPath()
    {
        static int counter = 0;
        return m_tempDir.path() + QString("/config_%1.ini").arg(++counter);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tempDir.isValid());
    }

    // ---- 默认值 ----
    void testDefaultValues()
    {
        ConfigManager cfg;
        cfg.loadConfig(tempConfigPath());

        QCOMPARE(cfg.serialBaudRate(), 9600);
        QCOMPARE(cfg.serialDataBits(), QSerialPort::Data8);
        QCOMPARE(cfg.serialParity(), QSerialPort::NoParity);
        QCOMPARE(cfg.serialStopBits(), QSerialPort::OneStop);
        QCOMPARE(cfg.serialFlowControl(), QSerialPort::NoFlowControl);
        QCOMPARE(cfg.networkProtocol(), QString("TCP"));
        QCOMPARE(cfg.serverAddress(), QString("127.0.0.1"));
        QCOMPARE(cfg.serverPort(), quint16(9000));
        QCOMPARE(cfg.autoUpload(), true);
        QCOMPARE(cfg.uploadInterval(), 5000);
        QCOMPARE(cfg.collectInterval(), 2000);
        QCOMPARE(cfg.databasePath(), QString("envmonitor.db"));
    }

    // ---- 串口配置读写 ----
    void testSerialConfig_readWrite()
    {
        QString path = tempConfigPath();
        {
            ConfigManager cfg;
            cfg.loadConfig(path);
            cfg.setSerialPortName("/dev/ttyUSB0");
            cfg.setSerialBaudRate(115200);
            cfg.setSerialDataBits(QSerialPort::Data7);
            cfg.setSerialParity(QSerialPort::EvenParity);
            cfg.setSerialStopBits(QSerialPort::TwoStop);
            cfg.setSerialFlowControl(QSerialPort::HardwareControl);
            cfg.saveConfig();
        }
        {
            ConfigManager cfg;
            cfg.loadConfig(path);
            QCOMPARE(cfg.serialPortName(), QString("/dev/ttyUSB0"));
            QCOMPARE(cfg.serialBaudRate(), qint32(115200));
            QCOMPARE(cfg.serialDataBits(), QSerialPort::Data7);
            QCOMPARE(cfg.serialParity(), QSerialPort::EvenParity);
            QCOMPARE(cfg.serialStopBits(), QSerialPort::TwoStop);
            QCOMPARE(cfg.serialFlowControl(), QSerialPort::HardwareControl);
        }
    }

    // ---- 网络配置读写 ----
    void testNetworkConfig_readWrite()
    {
        QString path = tempConfigPath();
        {
            ConfigManager cfg;
            cfg.loadConfig(path);
            cfg.setNetworkProtocol("UDP");
            cfg.setServerAddress("192.168.1.100");
            cfg.setServerPort(8080);
            cfg.setAutoUpload(false);
            cfg.setUploadInterval(10000);
            cfg.saveConfig();
        }
        {
            ConfigManager cfg;
            cfg.loadConfig(path);
            QCOMPARE(cfg.networkProtocol(), QString("UDP"));
            QCOMPARE(cfg.serverAddress(), QString("192.168.1.100"));
            QCOMPARE(cfg.serverPort(), quint16(8080));
            QCOMPARE(cfg.autoUpload(), false);
            QCOMPARE(cfg.uploadInterval(), 10000);
        }
    }

    // ---- 采集配置 ----
    void testCollectConfig_readWrite()
    {
        QString path = tempConfigPath();
        {
            ConfigManager cfg;
            cfg.loadConfig(path);
            cfg.setCollectInterval(5000);
            cfg.saveConfig();
        }
        {
            ConfigManager cfg;
            cfg.loadConfig(path);
            QCOMPARE(cfg.collectInterval(), 5000);
        }
    }

    // ---- 信号 ----
    void testSaveEmitsConfigChanged()
    {
        ConfigManager cfg;
        cfg.loadConfig(tempConfigPath());

        QSignalSpy spy(&cfg, &ConfigManager::configChanged);
        cfg.saveConfig();
        QCOMPARE(spy.count(), 1);
    }

    // ---- 重复加载不泄漏 ----
    void testReloadConfig()
    {
        ConfigManager cfg;
        QString path1 = tempConfigPath();
        QString path2 = tempConfigPath();

        cfg.loadConfig(path1);
        cfg.setSerialBaudRate(115200);
        cfg.saveConfig();

        cfg.loadConfig(path2);
        QCOMPARE(cfg.serialBaudRate(), qint32(9600));
    }

    // ---- 加载已有配置文件 ----
    void testLoadExistingConfig()
    {
        QString path = tempConfigPath();
        {
            ConfigManager cfg;
            cfg.loadConfig(path);
            cfg.setServerAddress("10.0.0.1");
            cfg.saveConfig();
        }
        {
            ConfigManager cfg;
            cfg.loadConfig(path);
            QCOMPARE(cfg.serverAddress(), QString("10.0.0.1"));
        }
    }
};

QTEST_APPLESS_MAIN(TestConfig)
#include "test_config.moc"
