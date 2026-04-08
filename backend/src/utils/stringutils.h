#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <QString>
#include <QDateTime>
#include <QByteArray>

/**
 * @brief 字符串工具类
 * 提供通用的字符串处理、格式化、转换功能
 */
class StringUtils
{
public:
    // 时间格式化
    static QString formatDateTime(const QDateTime &dt, const QString &format = "yyyy-MM-dd HH:mm:ss");
    static QString formatTimestamp(qint64 timestamp);
    static QDateTime parseDateTime(const QString &str, const QString &format = "yyyy-MM-dd HH:mm:ss");

    // 数值格式化
    static QString formatDouble(double value, int precision = 1);
    static QString formatTemperature(double value);
    static QString formatHumidity(double value);
    static QString formatLux(double value);
    static QString formatFileSize(qint64 bytes);

    // 字节数组转换
    static QString bytesToHex(const QByteArray &data, const QString &separator = " ");
    static QByteArray hexToBytes(const QString &hex);

    // 字符串验证
    static bool isValidIpAddress(const QString &ip);
    static bool isValidPort(int port);
    static bool isNumeric(const QString &str);

    // 字符串处理
    static QString truncate(const QString &str, int maxLength, const QString &suffix = "...");
    static QString padLeft(const QString &str, int width, QChar fill = ' ');
    static QString padRight(const QString &str, int width, QChar fill = ' ');

private:
    StringUtils() = default; // 禁止实例化
};

#endif // STRINGUTILS_H
