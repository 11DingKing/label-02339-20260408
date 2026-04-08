#include "serialmanager.h"
#include <QDebug>
#include <QRandomGenerator>

/**
 * @brief 串口管理器构造函数
 * @param parent 父对象指针
 * 
 * 串口管理器负责：
 * 1. 串口设备的打开、关闭、参数配置
 * 2. 串口数据的发送和接收
 * 3. 数据帧的解析（按换行符分割）
 * 4. 模拟模式下生成随机传感器数据
 * 5. 连接状态监测和错误处理
 */
SerialManager::SerialManager(QObject *parent)
    : QObject(parent)
    , m_serial(new QSerialPort(this))
    , m_simTimer(new QTimer(this))
{
    // 连接串口信号
    connect(m_serial, &QSerialPort::readyRead, this, &SerialManager::onReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialManager::onErrorOccurred);
    
    // 连接模拟定时器
    connect(m_simTimer, &QTimer::timeout, this, &SerialManager::onSimulationTick);
}

SerialManager::~SerialManager()
{
    stopSimulation();
    closePort();
}

/**
 * @brief 打开串口并配置参数
 * @param portName 串口名称（如 COM3 或 /dev/ttyUSB0）
 * @param baudRate 波特率
 * @param dataBits 数据位
 * @param parity 校验位
 * @param stopBits 停止位
 * @param flowControl 流控制
 * @return 成功返回true，失败返回false
 */
bool SerialManager::openPort(const QString &portName, qint32 baudRate,
                              QSerialPort::DataBits dataBits,
                              QSerialPort::Parity parity,
                              QSerialPort::StopBits stopBits,
                              QSerialPort::FlowControl flowControl)
{
    // 如果已打开，先关闭
    if (m_serial->isOpen()) {
        closePort();
    }

    // 配置串口参数
    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(dataBits);
    m_serial->setParity(parity);
    m_serial->setStopBits(stopBits);
    m_serial->setFlowControl(flowControl);

    // 尝试打开串口
    if (!m_serial->open(QIODevice::ReadWrite)) {
        QString err = m_serial->errorString();
        qWarning() << "[SerialManager] 串口打开失败:" << portName << err;
        
        // 提供详细的错误提示
        QString detailMsg;
        if (err.contains("Permission denied") || err.contains("Access denied")) {
            detailMsg = QString("串口 %1 打开失败：权限不足\n\n"
                               "可能原因：\n"
                               "• 串口被其他程序占用\n"
                               "• 当前用户没有串口访问权限\n\n"
                               "解决建议：\n"
                               "• 关闭其他串口调试工具\n"
                               "• Linux/macOS 用户尝试: sudo chmod 666 %1").arg(portName);
        } else if (err.contains("No such file") || err.contains("not found")) {
            detailMsg = QString("串口 %1 打开失败：设备不存在\n\n"
                               "可能原因：\n"
                               "• 设备未连接或已断开\n"
                               "• 设备驱动未安装\n\n"
                               "解决建议：\n"
                               "• 检查USB连接是否牢固\n"
                               "• 点击「刷新」重新获取串口列表").arg(portName);
        } else {
            detailMsg = QString("串口 %1 打开失败：%2\n\n"
                               "解决建议：\n"
                               "• 检查设备连接\n"
                               "• 尝试重新插拔设备").arg(portName, err);
        }
        
        emit errorOccurred(detailMsg);
        emit connectionStatusChanged(false);
        return false;
    }

    qInfo() << "[SerialManager] 串口已打开:" << portName << "波特率:" << baudRate;
    emit connectionStatusChanged(true);
    return true;
}

/**
 * @brief 关闭串口
 */
void SerialManager::closePort()
{
    if (m_serial->isOpen()) {
        m_serial->close();
        qInfo() << "[SerialManager] 串口已关闭";
        emit connectionStatusChanged(false);
    }
}

bool SerialManager::isConnected() const
{
    return m_serial->isOpen();
}

/**
 * @brief 发送数据到串口
 * @param data 要发送的数据
 * @return 成功返回true，失败返回false
 */
bool SerialManager::sendData(const QByteArray &data)
{
    if (!m_serial->isOpen()) {
        emit errorOccurred("发送失败：串口未打开\n\n请先连接串口后再发送数据");
        return false;
    }

    qint64 written = m_serial->write(data);
    if (written == -1) {
        emit errorOccurred(QString("数据发送失败：%1\n\n请检查设备连接状态").arg(m_serial->errorString()));
        return false;
    }

    qDebug() << "[SerialManager] 已发送" << written << "字节";
    return true;
}

/**
 * @brief 获取可用串口列表
 * @return 串口名称列表（格式：端口名 (描述)）
 */
QStringList SerialManager::availablePorts()
{
    QStringList ports;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        ports << QString("%1 (%2)").arg(info.portName(), info.description());
    }
    return ports;
}

/**
 * @brief 串口数据接收槽函数
 * 
 * 数据接收处理流程：
 * 1. 将接收到的数据追加到缓冲区
 * 2. 按换行符分割完整数据帧
 * 3. 解析数据帧为 SensorData 对象
 * 4. 发送信号通知上层处理
 */
void SerialManager::onReadyRead()
{
    m_buffer.append(m_serial->readAll());

    // 按换行符分割完整帧
    while (m_buffer.contains('\n')) {
        int idx = m_buffer.indexOf('\n');
        QByteArray frame = m_buffer.left(idx).trimmed();
        m_buffer.remove(0, idx + 1);

        if (frame.isEmpty()) continue;

        emit rawDataReceived(frame);

        // 解析数据帧
        SensorData data = parseData(frame);
        if (data.isValid()) {
            qDebug() << "[SerialManager] 解析成功:" << data.toString();
            emit dataReceived(data);
        } else {
            qWarning() << "[SerialManager] 数据格式错误:" << frame;
            emit errorOccurred(QString("数据格式错误：%1\n\n"
                                       "期望格式：Temp:25.5,Hum:60.0,Lux:800").arg(QString(frame)));
        }
    }
}

/**
 * @brief 串口错误处理槽函数
 * @param error 错误类型
 */
void SerialManager::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;

    if (error == QSerialPort::ResourceError) {
        // 设备断开连接
        qWarning() << "[SerialManager] 串口断开连接";
        emit portDisconnected();
        emit connectionStatusChanged(false);
        closePort();
    } else {
        qWarning() << "[SerialManager] 串口错误:" << m_serial->errorString();
        emit errorOccurred(QString("串口通信错误：%1").arg(m_serial->errorString()));
    }
}

/**
 * @brief 解析原始数据为 SensorData 对象
 * @param raw 原始数据字节数组
 * @return 解析后的 SensorData 对象
 */
SensorData SerialManager::parseData(const QByteArray &raw)
{
    return SensorData::fromRawString(QString::fromUtf8(raw));
}

// ==================== 模拟模式 ====================

/**
 * @brief 启动模拟模式
 * @param intervalMs 数据生成间隔（毫秒）
 * 
 * 模拟模式用于在没有真实传感器设备时测试系统功能。
 * 系统会按指定间隔生成随机的环境数据。
 */
void SerialManager::startSimulation(int intervalMs)
{
    m_simulating = true;
    m_simTimer->start(intervalMs);
    qInfo() << "[SerialManager] 模拟模式已启动, 间隔:" << intervalMs << "ms";
    emit connectionStatusChanged(true);
}

/**
 * @brief 停止模拟模式
 */
void SerialManager::stopSimulation()
{
    if (m_simulating) {
        m_simTimer->stop();
        m_simulating = false;
        qInfo() << "[SerialManager] 模拟模式已停止";
        emit connectionStatusChanged(false);
    }
}

/**
 * @brief 模拟数据生成定时器回调
 * 
 * 生成随机的传感器数据：
 * - 温度：20.0 ~ 35.0 ℃
 * - 湿度：40.0 ~ 80.0 %
 * - 光照：200 ~ 2000 Lux
 */
void SerialManager::onSimulationTick()
{
    // 使用 Qt 随机数生成器
    auto *rng = QRandomGenerator::global();
    
    // 生成随机数据（模拟真实传感器的波动）
    double temp = 20.0 + rng->bounded(150) / 10.0;   // 20.0 ~ 35.0
    double hum  = 40.0 + rng->bounded(400) / 10.0;   // 40.0 ~ 80.0
    double lux  = 200.0 + rng->bounded(1800);        // 200 ~ 2000

    SensorData data(temp, hum, lux);

    // 生成原始数据字符串（模拟串口接收格式）
    QString raw = QString("Temp:%1,Hum:%2,Lux:%3")
                      .arg(temp, 0, 'f', 1).arg(hum, 0, 'f', 1).arg(lux, 0, 'f', 1);
    emit rawDataReceived(raw.toUtf8());
    emit dataReceived(data);
}
