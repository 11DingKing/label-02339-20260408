#include "stringutils.h"
#include <QRegularExpression>

QString StringUtils::formatDateTime(const QDateTime &dt, const QString &format)
{
    return dt.toString(format);
}

QString StringUtils::formatTimestamp(qint64 timestamp)
{
    return QDateTime::fromMSecsSinceEpoch(timestamp).toString("yyyy-MM-dd HH:mm:ss");
}

QDateTime StringUtils::parseDateTime(const QString &str, const QString &format)
{
    return QDateTime::fromString(str, format);
}

QString StringUtils::formatDouble(double value, int precision)
{
    return QString::number(value, 'f', precision);
}

QString StringUtils::formatTemperature(double value)
{
    return QString("%1 ℃").arg(value, 0, 'f', 1);
}

QString StringUtils::formatHumidity(double value)
{
    return QString("%1 %").arg(value, 0, 'f', 1);
}

QString StringUtils::formatLux(double value)
{
    return QString("%1 Lux").arg(value, 0, 'f', 0);
}

QString StringUtils::formatFileSize(qint64 bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    return QString("%1 %2").arg(size, 0, 'f', unitIndex > 0 ? 2 : 0).arg(units[unitIndex]);
}

QString StringUtils::bytesToHex(const QByteArray &data, const QString &separator)
{
    QStringList hexList;
    for (char byte : data) {
        hexList << QString("%1").arg(static_cast<unsigned char>(byte), 2, 16, QChar('0')).toUpper();
    }
    return hexList.join(separator);
}

QByteArray StringUtils::hexToBytes(const QString &hex)
{
    QString cleanHex = hex;
    cleanHex.remove(QRegularExpression("[^0-9A-Fa-f]"));

    QByteArray result;
    for (int i = 0; i + 1 < cleanHex.length(); i += 2) {
        bool ok;
        result.append(static_cast<char>(cleanHex.mid(i, 2).toInt(&ok, 16)));
    }
    return result;
}

bool StringUtils::isValidIpAddress(const QString &ip)
{
    QRegularExpression ipRegex(
        R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)"
    );
    return ipRegex.match(ip).hasMatch() || ip == "localhost";
}

bool StringUtils::isValidPort(int port)
{
    return port >= 1 && port <= 65535;
}

bool StringUtils::isNumeric(const QString &str)
{
    QRegularExpression numRegex(R"(^-?\d+\.?\d*$)");
    return numRegex.match(str).hasMatch();
}

QString StringUtils::truncate(const QString &str, int maxLength, const QString &suffix)
{
    if (str.length() <= maxLength) {
        return str;
    }
    return str.left(maxLength - suffix.length()) + suffix;
}

QString StringUtils::padLeft(const QString &str, int width, QChar fill)
{
    if (str.length() >= width) {
        return str;
    }
    return QString(width - str.length(), fill) + str;
}

QString StringUtils::padRight(const QString &str, int width, QChar fill)
{
    if (str.length() >= width) {
        return str;
    }
    return str + QString(width - str.length(), fill);
}
