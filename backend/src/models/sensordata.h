#ifndef SENSORDATA_H
#define SENSORDATA_H

#include <QString>
#include <QDateTime>
#include <QVariant>

/**
 * @brief 传感器数据模型类
 * 封装环境参数数据：温度、湿度、光照强度
 */
class SensorData
{
public:
    SensorData();
    SensorData(double temp, double hum, double lux);

    // Getters
    int id() const { return m_id; }
    QDateTime collectTime() const { return m_collectTime; }
    double temperature() const { return m_temperature; }
    double humidity() const { return m_humidity; }
    double lightIntensity() const { return m_lightIntensity; }

    // Setters
    void setId(int id) { m_id = id; }
    void setCollectTime(const QDateTime &time) { m_collectTime = time; }
    void setTemperature(double temp) { m_temperature = temp; }
    void setHumidity(double hum) { m_humidity = hum; }
    void setLightIntensity(double lux) { m_lightIntensity = lux; }

    // 工具方法
    QString toString() const;
    QString toUploadString() const;
    bool isValid() const;

    // 从原始串口字符串解析
    static SensorData fromRawString(const QString &raw);

private:
    int m_id = -1;
    QDateTime m_collectTime;
    double m_temperature = 0.0;
    double m_humidity = 0.0;
    double m_lightIntensity = 0.0;
};

Q_DECLARE_METATYPE(SensorData)

#endif // SENSORDATA_H
