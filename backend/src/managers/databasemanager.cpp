#include "databasemanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::DatabaseManager(const QString &connectionName, QObject *parent)
    : QObject(parent)
    , m_connectionName(connectionName)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
        qInfo() << "[DatabaseManager] 数据库连接已关闭";
    }
    // 移除命名连接，避免连接泄漏
    if (!m_connectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool DatabaseManager::initialize(const QString &dbPath)
{
    // 如果没有预设连接名，生成唯一连接名
    if (m_connectionName.isEmpty()) {
        m_connectionName = QString("envmonitor_%1").arg(
            reinterpret_cast<quintptr>(this), 0, 16);
    }

    // 如果已有同名连接先移除
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        QString err = m_db.lastError().text();
        qCritical() << "[DatabaseManager] 数据库连接失败:" << err;
        emit databaseError("数据库连接失败: " + err);
        m_connected = false;
        return false;
    }

    m_connected = true;
    qInfo() << "[DatabaseManager] 数据库连接成功:" << dbPath;

    if (!createTable()) {
        return false;
    }

    return true;
}

bool DatabaseManager::isConnected() const
{
    return m_connected && m_db.isOpen();
}

bool DatabaseManager::createTable()
{
    QSqlQuery query(m_db);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS environment_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            collect_time TEXT NOT NULL,
            temperature REAL NOT NULL,
            humidity REAL NOT NULL,
            light_intensity REAL NOT NULL
        )
    )";

    if (!query.exec(sql)) {
        QString err = query.lastError().text();
        qCritical() << "[DatabaseManager] 建表失败:" << err;
        emit databaseError("建表失败: " + err);
        return false;
    }

    // 创建时间索引以加速查询
    query.exec("CREATE INDEX IF NOT EXISTS idx_collect_time ON environment_data(collect_time)");

    qInfo() << "[DatabaseManager] 数据表初始化完成";
    return true;
}

bool DatabaseManager::insertRecord(const SensorData &data)
{
    if (!isConnected()) {
        emit databaseError("数据库未连接");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO environment_data (collect_time, temperature, humidity, light_intensity)
        VALUES (:time, :temp, :hum, :lux)
    )");
    query.bindValue(":time", data.collectTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.bindValue(":temp", data.temperature());
    query.bindValue(":hum", data.humidity());
    query.bindValue(":lux", data.lightIntensity());

    if (!query.exec()) {
        QString err = query.lastError().text();
        qWarning() << "[DatabaseManager] 插入记录失败:" << err;
        emit databaseError("插入记录失败: " + err);
        return false;
    }

    qDebug() << "[DatabaseManager] 记录已插入, Temp:" << data.temperature()
             << "Hum:" << data.humidity() << "Lux:" << data.lightIntensity();
    emit recordInserted();
    return true;
}

QList<SensorData> DatabaseManager::queryByTimeRange(const QDateTime &from, const QDateTime &to)
{
    QList<SensorData> results;
    if (!isConnected()) {
        emit databaseError("数据库未连接");
        return results;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, collect_time, temperature, humidity, light_intensity
        FROM environment_data
        WHERE collect_time BETWEEN :from AND :to
        ORDER BY collect_time DESC
    )");
    query.bindValue(":from", from.toString("yyyy-MM-dd HH:mm:ss"));
    query.bindValue(":to", to.toString("yyyy-MM-dd HH:mm:ss"));

    if (!query.exec()) {
        qWarning() << "[DatabaseManager] 查询失败:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        SensorData data;
        data.setId(query.value(0).toInt());
        data.setCollectTime(QDateTime::fromString(query.value(1).toString(), "yyyy-MM-dd HH:mm:ss"));
        data.setTemperature(query.value(2).toDouble());
        data.setHumidity(query.value(3).toDouble());
        data.setLightIntensity(query.value(4).toDouble());
        results.append(data);
    }

    qInfo() << "[DatabaseManager] 查询到" << results.size() << "条记录";
    return results;
}

QList<SensorData> DatabaseManager::queryLatest(int count)
{
    QList<SensorData> results;
    if (!isConnected()) return results;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, collect_time, temperature, humidity, light_intensity
        FROM environment_data
        ORDER BY id DESC
        LIMIT :count
    )");
    query.bindValue(":count", count);

    if (query.exec()) {
        while (query.next()) {
            SensorData data;
            data.setId(query.value(0).toInt());
            data.setCollectTime(QDateTime::fromString(query.value(1).toString(), "yyyy-MM-dd HH:mm:ss"));
            data.setTemperature(query.value(2).toDouble());
            data.setHumidity(query.value(3).toDouble());
            data.setLightIntensity(query.value(4).toDouble());
            results.append(data);
        }
    }
    return results;
}

int DatabaseManager::deleteByTimeRange(const QDateTime &from, const QDateTime &to)
{
    if (!isConnected()) {
        emit databaseError("数据库未连接");
        return -1;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        DELETE FROM environment_data
        WHERE collect_time BETWEEN :from AND :to
    )");
    query.bindValue(":from", from.toString("yyyy-MM-dd HH:mm:ss"));
    query.bindValue(":to", to.toString("yyyy-MM-dd HH:mm:ss"));

    if (!query.exec()) {
        qWarning() << "[DatabaseManager] 删除失败:" << query.lastError().text();
        emit databaseError("删除失败: " + query.lastError().text());
        return -1;
    }

    int affected = query.numRowsAffected();
    qInfo() << "[DatabaseManager] 已删除" << affected << "条记录";
    return affected;
}

int DatabaseManager::recordCount() const
{
    if (!m_connected) return 0;
    QSqlQuery query(m_db);
    query.exec("SELECT COUNT(*) FROM environment_data");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}
