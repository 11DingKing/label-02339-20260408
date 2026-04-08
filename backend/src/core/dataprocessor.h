#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include <QObject>
#include "../models/sensordata.h"
#include "../managers/databasemanager.h"
#include "../managers/networkmanager.h"

/**
 * @brief 数据处理器
 * 负责接收原始数据、处理后分发到数据库和网络模块
 */
class DataProcessor : public QObject
{
    Q_OBJECT

public:
    explicit DataProcessor(QObject *parent = nullptr);

    void setDatabaseManager(DatabaseManager *dbMgr);
    void setNetworkManager(NetworkManager *netMgr);

    SensorData latestData() const { return m_latestData; }

public slots:
    // 处理从串口接收到的数据
    void processIncomingData(const SensorData &data);

signals:
    void dataUpdated(const SensorData &data);

private:
    DatabaseManager *m_dbManager = nullptr;
    NetworkManager *m_netManager = nullptr;
    SensorData m_latestData;
};

#endif // DATAPROCESSOR_H
