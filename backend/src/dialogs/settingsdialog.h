#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // 串口设置
    QString portName() const;
    int baudRate() const;
    int dataBits() const;
    int parityIndex() const;
    int stopBitsIndex() const;
    int flowControlIndex() const;

    // 网络设置
    QString protocol() const;
    QString serverAddress() const;
    int serverPort() const;
    bool autoUpload() const;
    int uploadInterval() const;

    // 采集设置
    int collectInterval() const;

    // 设置值
    void setPortList(const QStringList &ports);
    void setBaudRate(int rate);
    void setServerAddress(const QString &addr);
    void setServerPort(int port);
    void setProtocol(const QString &proto);
    void setAutoUpload(bool enabled);
    void setUploadInterval(int ms);
    void setCollectInterval(int ms);
    
    // Toast提示
    void showToast(const QString &message, int duration = 2000);

signals:
    void refreshPortsRequested();
    void serialConnectRequested();
    void serialDisconnectRequested();
    void simulateRequested();
    void serialSendRequested(const QString &data, bool hexMode);
    void networkConnectRequested();
    void networkDisconnectRequested();
    void networkReconnectRequested();
    void manualUploadRequested();
    void collectIntervalChanged(int ms);

public slots:
    void setSerialConnected(bool connected);
    void setNetworkConnected(bool connected);
    void setSimulating(bool simulating);

private:
    void setupUI();
    void setupSerialTab(QWidget *tab);
    void setupNetworkTab(QWidget *tab);
    void setupCollectTab(QWidget *tab);
    void applyStyles();

    // 串口控件
    QComboBox *m_portCombo;
    QComboBox *m_baudCombo;
    QComboBox *m_dataBitsCombo;
    QComboBox *m_parityCombo;
    QComboBox *m_stopBitsCombo;
    QComboBox *m_flowCombo;
    QPushButton *m_refreshBtn;
    QPushButton *m_serialConnectBtn;
    QPushButton *m_serialDisconnectBtn;
    QPushButton *m_simulateBtn;
    QLineEdit *m_sendEdit;
    QCheckBox *m_hexModeCheck;
    QPushButton *m_sendBtn;

    // 网络控件
    QComboBox *m_protocolCombo;
    QLineEdit *m_serverAddrEdit;
    QSpinBox *m_serverPortSpin;
    QCheckBox *m_autoUploadCheck;
    QSpinBox *m_uploadIntervalSpin;
    QPushButton *m_netConnectBtn;
    QPushButton *m_netDisconnectBtn;
    QPushButton *m_netReconnectBtn;
    QPushButton *m_manualUploadBtn;

    // 采集控件
    QSpinBox *m_collectIntervalSpin;
    
    // Toast控件
    QLabel *m_toastLabel;
    QTimer *m_toastTimer;
};

#endif // SETTINGSDIALOG_H
