#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include "../models/sensordata.h"

/**
 * @brief SQLite 数据库管理器
 * 负责环境参数数据的增删查操作
 */
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    explicit DatabaseManager(const QString &connectionName, QObject *parent = nullptr);
    ~DatabaseManager();

    // 初始化数据库连接与建表
    bool initialize(const QString &dbPath);
    bool isConnected() const;

    // 增：插入一条采集记录
    bool insertRecord(const SensorData &data);

    // 查：按时间范围查询
    QList<SensorData> queryByTimeRange(const QDateTime &from, const QDateTime &to);

    // 查：获取最近 N 条记录
    QList<SensorData> queryLatest(int count = 100);

    // 删：删除指定时间段的数据
    int deleteByTimeRange(const QDateTime &from, const QDateTime &to);

    // 获取记录总数
    int recordCount() const;

signals:
    void databaseError(const QString &error);
    void recordInserted();

private:
    bool createTable();
    QSqlDatabase m_db;
    QString m_connectionName;
    bool m_connected = false;
};

#endif // DATABASEMANAGER_H
