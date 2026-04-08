#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QTimer>
#include <QTextEdit>

#include "managers/serialmanager.h"
#include "managers/networkmanager.h"
#include "managers/databasemanager.h"
#include "managers/configmanager.h"
#include "core/dataprocessor.h"
#include "widgets/gaugewidget.h"

class SettingsDialog;

/**
 * @brief 主窗口类
 * 负责整体 UI 布局、控件绑定、用户交互
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 数据更新
    void onDataUpdated(const SensorData &data);

    // 串口操作
    void onConnectSerial();
    void onDisconnectSerial();
    void onStartSimulation();
    void onSerialStatusChanged(bool connected);
    void onSerialDisconnected();
    void onSerialSend(const QString &data, bool hexMode);

    // 网络操作
    void onConnectNetwork();
    void onDisconnectNetwork();
    void onManualUpload();
    void onNetworkStatusChanged(bool connected);
    void onResetReconnect();

    // 数据库操作
    void onQueryHistory();
    void onDeleteHistory();

    // 菜单操作
    void onSaveConfig();
    void onLoadConfig();
    void onAbout();

    // 状态更新
    void onSerialError(const QString &error);
    void onNetworkError(const QString &error);
    void onDatabaseError(const QString &error);
    void onUploadSuccess();
    void onUploadFailed(const QString &error);

    // 采集设置
    void onCollectIntervalChanged(int ms);
    
    // 一键模拟
    void onQuickSimulate();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    QWidget* createRealtimePanel();
    QWidget* createHistoryPanel();
    QWidget* createLogPanel();
    void showSettingsDialog();

    void initManagers();
    void connectSignals();
    void loadConfigToUI();
    void applyStyleSheet();
    void showMessage(const QString &msg, bool isError = false);
    void appendLog(const QString &msg);

    // 管理器
    SerialManager *m_serialMgr;
    NetworkManager *m_networkMgr;
    DatabaseManager *m_dbMgr;
    ConfigManager *m_configMgr;
    DataProcessor *m_dataProcessor;

    // 设置对话框
    SettingsDialog *m_settingsDialog;

    // 实时数据展示
    GaugeWidget *m_tempGauge;
    GaugeWidget *m_humGauge;
    GaugeWidget *m_luxGauge;
    QProgressBar *m_tempBar;
    QProgressBar *m_humBar;
    QProgressBar *m_luxBar;
    QLabel *m_tempValueLabel;
    QLabel *m_humValueLabel;
    QLabel *m_luxValueLabel;
    QLabel *m_lastUpdateLabel;

    // 历史数据
    QTableWidget *m_historyTable;
    QDateTimeEdit *m_queryFrom;
    QDateTimeEdit *m_queryTo;
    QLabel *m_recordCountLabel;

    // 日志
    QTextEdit *m_logView;

    // 状态栏
    QLabel *m_serialStatusLabel;
    QLabel *m_networkStatusLabel;
    QLabel *m_dbStatusLabel;
};

#endif // MAINWINDOW_H
