#include "dataprocessor.h"
#include <QDebug>

/**
 * @brief 数据处理器构造函数
 * @param parent 父对象指针
 * 
 * 数据处理器是系统的核心协调模块，负责：
 * 1. 接收串口管理器解析后的传感器数据
 * 2. 验证数据有效性
 * 3. 将数据存储到数据库
 * 4. 将数据传递给网络管理器用于上传
 * 5. 通知UI层更新显示
 */
DataProcessor::DataProcessor(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief 设置数据库管理器
 * @param dbMgr 数据库管理器指针
 */
void DataProcessor::setDatabaseManager(DatabaseManager *dbMgr)
{
    m_dbManager = dbMgr;
}

/**
 * @brief 设置网络管理器
 * @param netMgr 网络管理器指针
 */
void DataProcessor::setNetworkManager(NetworkManager *netMgr)
{
    m_netManager = netMgr;
}

/**
 * @brief 处理接收到的传感器数据
 * @param data 传感器数据对象
 * 
 * 数据处理流程：
 * 1. 验证数据有效性（温度、湿度、光照是否在合理范围内）
 * 2. 更新最新数据缓存
 * 3. 将数据写入SQLite数据库持久化存储
 * 4. 将数据传递给网络管理器，供自动上传功能使用
 * 5. 发送dataUpdated信号，通知UI层更新仪表盘显示
 * 
 * 注意：无效数据会被丢弃并记录警告日志
 */
void DataProcessor::processIncomingData(const SensorData &data)
{
    // 步骤1：验证数据有效性
    if (!data.isValid()) {
        qWarning() << "[DataProcessor] 收到无效数据，已丢弃:"
                    << "Temp=" << data.temperature()
                    << "Hum=" << data.humidity()
                    << "Lux=" << data.lightIntensity();
        return;
    }

    // 步骤2：更新最新数据缓存
    m_latestData = data;

    // 步骤3：存入数据库
    if (m_dbManager && m_dbManager->isConnected()) {
        if (!m_dbManager->insertRecord(data)) {
            qWarning() << "[DataProcessor] 数据库写入失败";
        }
    } else if (m_dbManager && !m_dbManager->isConnected()) {
        qWarning() << "[DataProcessor] 数据库未连接，跳过存储";
    }

    // 步骤4：更新网络管理器的最新数据（供自动上传使用）
    if (m_netManager) {
        m_netManager->setLatestData(data);
    }

    qDebug() << "[DataProcessor] 数据已处理: Temp=" << data.temperature()
             << "Hum=" << data.humidity() << "Lux=" << data.lightIntensity()
             << "Time=" << data.collectTime().toString("HH:mm:ss");

    // 步骤5：通知UI更新显示
    emit dataUpdated(data);
}
