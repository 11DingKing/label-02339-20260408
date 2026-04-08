#include "sensordata.h"
#include <QStringList>
#include <QRegularExpression>

SensorData::SensorData()
    : m_collectTime(QDateTime::currentDateTime())
{
}

SensorData::SensorData(double temp, double hum, double lux)
    : m_collectTime(QDateTime::currentDateTime())
    , m_temperature(temp)
    , m_humidity(hum)
    , m_lightIntensity(lux)
{
}

QString SensorData::toString() const
{
    return QString("Time:%1 Temp:%2 Hum:%3 Lux:%4")
        .arg(m_collectTime.toString("yyyy-MM-dd HH:mm:ss"))
        .arg(m_temperature, 0, 'f', 1)
        .arg(m_humidity, 0, 'f', 1)
        .arg(m_lightIntensity, 0, 'f', 1);
}

QString SensorData::toUploadString() const
{
    return QString("Temp:%1,Hum:%2,Lux:%3,Time:%4")
        .arg(m_temperature, 0, 'f', 1)
        .arg(m_humidity, 0, 'f', 1)
        .arg(m_lightIntensity, 0, 'f', 1)
        .arg(m_collectTime.toString("yyyy-MM-dd HH:mm:ss"));
}

bool SensorData::isValid() const
{
    return m_temperature >= -50.0 && m_temperature <= 100.0
        && m_humidity >= 0.0 && m_humidity <= 100.0
        && m_lightIntensity >= 0.0 && m_lightIntensity <= 200000.0;
}

SensorData SensorData::fromRawString(const QString &raw)
{
    SensorData data;
    // 解析格式: "Temp:25.5,Hum:60.0,Lux:800"
    QString trimmed = raw.trimmed();
    QStringList pairs = trimmed.split(',', Qt::SkipEmptyParts);

    for (const QString &pair : pairs) {
        QStringList kv = pair.split(':', Qt::SkipEmptyParts);
        if (kv.size() != 2) continue;

        QString key = kv[0].trimmed().toLower();
        bool ok = false;
        double value = kv[1].trimmed().toDouble(&ok);
        if (!ok) continue;

        if (key == "temp") {
            data.setTemperature(value);
        } else if (key == "hum") {
            data.setHumidity(value);
        } else if (key == "lux") {
            data.setLightIntensity(value);
        }
    }

    data.setCollectTime(QDateTime::currentDateTime());
    return data;
}
