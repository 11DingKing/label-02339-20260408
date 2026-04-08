#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_toastLabel(nullptr)
    , m_toastTimer(nullptr)
{
    setWindowTitle("系统设置");
    setMinimumSize(520, 540);
    setModal(false);
    setupUI();
    applyStyles();
    
    // 初始化Toast
    m_toastLabel = new QLabel(this);
    m_toastLabel->setAlignment(Qt::AlignCenter);
    m_toastLabel->setStyleSheet(R"(
        QLabel {
            background: rgba(30, 41, 59, 0.9);
            color: white;
            padding: 12px 24px;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 500;
        }
    )");
    m_toastLabel->hide();
    
    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, this, [this]() {
        m_toastLabel->hide();
    });
}

void SettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QTabWidget *tabs = new QTabWidget();
    
    QWidget *serialTab = new QWidget();
    QWidget *networkTab = new QWidget();
    QWidget *collectTab = new QWidget();
    
    setupSerialTab(serialTab);
    setupNetworkTab(networkTab);
    setupCollectTab(collectTab);
    
    tabs->addTab(serialTab, "串口设置");
    tabs->addTab(networkTab, "网络设置");
    tabs->addTab(collectTab, "采集设置");
    
    mainLayout->addWidget(tabs);

    // 关闭按钮
    QPushButton *closeBtn = new QPushButton("关闭");
    closeBtn->setFixedHeight(40);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    mainLayout->addWidget(closeBtn);
}

void SettingsDialog::setupSerialTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(12);
    layout->setContentsMargins(12, 12, 12, 12);

    // 连接参数 - 紧凑双列布局
    QGroupBox *paramGroup = new QGroupBox("连接参数");
    QGridLayout *grid = new QGridLayout(paramGroup);
    grid->setSpacing(10);
    grid->setContentsMargins(14, 22, 14, 14);

    // 第一行：串口 + 刷新
    QLabel *portLabel = new QLabel("串口");
    portLabel->setFixedWidth(50);
    grid->addWidget(portLabel, 0, 0);
    
    QHBoxLayout *portRow = new QHBoxLayout();
    portRow->setSpacing(6);
    m_portCombo = new QComboBox();
    m_portCombo->setMinimumHeight(34);
    portRow->addWidget(m_portCombo, 1);
    m_refreshBtn = new QPushButton("刷新");
    m_refreshBtn->setFixedSize(60, 34);
    m_refreshBtn->setToolTip("刷新串口列表");
    connect(m_refreshBtn, &QPushButton::clicked, this, [this]() {
        showToast("串口列表已刷新");
        emit refreshPortsRequested();
    });
    portRow->addWidget(m_refreshBtn);
    grid->addLayout(portRow, 0, 1, 1, 3);

    // 第二行：波特率
    QLabel *baudLabel = new QLabel("波特率");
    baudLabel->setFixedWidth(50);
    grid->addWidget(baudLabel, 1, 0);
    m_baudCombo = new QComboBox();
    m_baudCombo->addItems({"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"});
    m_baudCombo->setCurrentText("9600");
    m_baudCombo->setMinimumHeight(34);
    grid->addWidget(m_baudCombo, 1, 1, 1, 3);

    // 第三行：数据位 + 校验位（同行）
    QLabel *dataLabel = new QLabel("数据位");
    dataLabel->setFixedWidth(50);
    grid->addWidget(dataLabel, 2, 0);
    m_dataBitsCombo = new QComboBox();
    m_dataBitsCombo->addItems({"5", "6", "7", "8"});
    m_dataBitsCombo->setCurrentText("8");
    m_dataBitsCombo->setMinimumHeight(34);
    grid->addWidget(m_dataBitsCombo, 2, 1);

    QLabel *parityLabel = new QLabel("校验");
    parityLabel->setFixedWidth(36);
    parityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(parityLabel, 2, 2);
    m_parityCombo = new QComboBox();
    m_parityCombo->addItems({"无", "偶", "奇"});
    m_parityCombo->setMinimumHeight(34);
    grid->addWidget(m_parityCombo, 2, 3);

    // 第四行：停止位 + 流控（同行）
    QLabel *stopLabel = new QLabel("停止位");
    stopLabel->setFixedWidth(50);
    grid->addWidget(stopLabel, 3, 0);
    m_stopBitsCombo = new QComboBox();
    m_stopBitsCombo->addItems({"1", "1.5", "2"});
    m_stopBitsCombo->setMinimumHeight(34);
    grid->addWidget(m_stopBitsCombo, 3, 1);

    QLabel *flowLabel = new QLabel("流控");
    flowLabel->setFixedWidth(36);
    flowLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(flowLabel, 3, 2);
    m_flowCombo = new QComboBox();
    m_flowCombo->addItems({"无", "硬件", "软件"});
    m_flowCombo->setMinimumHeight(34);
    grid->addWidget(m_flowCombo, 3, 3);

    // 设置列宽比例
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(3, 1);

    layout->addWidget(paramGroup);

    // 操作按钮 - 紧凑排列
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);
    
    m_serialConnectBtn = new QPushButton("连接");
    m_serialConnectBtn->setMinimumHeight(38);
    m_serialConnectBtn->setObjectName("primaryBtn");
    connect(m_serialConnectBtn, &QPushButton::clicked, this, [this]() {
        showToast("正在连接串口...");
        emit serialConnectRequested();
    });
    btnLayout->addWidget(m_serialConnectBtn);

    m_serialDisconnectBtn = new QPushButton("断开");
    m_serialDisconnectBtn->setMinimumHeight(38);
    m_serialDisconnectBtn->setEnabled(false);
    connect(m_serialDisconnectBtn, &QPushButton::clicked, this, [this]() {
        showToast("串口已断开");
        emit serialDisconnectRequested();
    });
    btnLayout->addWidget(m_serialDisconnectBtn);

    m_simulateBtn = new QPushButton("模拟");
    m_simulateBtn->setMinimumHeight(38);
    m_simulateBtn->setObjectName("successBtn");
    m_simulateBtn->setToolTip("启动模拟数据生成");
    connect(m_simulateBtn, &QPushButton::clicked, this, [this]() {
        showToast(m_simulateBtn->text() == "模拟" ? "模拟模式启动中..." : "模拟模式停止中...");
        emit simulateRequested();
    });
    btnLayout->addWidget(m_simulateBtn);

    layout->addLayout(btnLayout);

    // 数据发送 - 紧凑设计
    QGroupBox *sendGroup = new QGroupBox("数据发送");
    QVBoxLayout *sendLayout = new QVBoxLayout(sendGroup);
    sendLayout->setSpacing(8);
    sendLayout->setContentsMargins(14, 22, 14, 14);

    QHBoxLayout *modeRow = new QHBoxLayout();
    m_hexModeCheck = new QCheckBox("HEX模式");
    m_hexModeCheck->setToolTip("以十六进制格式发送数据");
    modeRow->addWidget(m_hexModeCheck);
    modeRow->addStretch();
    sendLayout->addLayout(modeRow);

    QHBoxLayout *sendRow = new QHBoxLayout();
    sendRow->setSpacing(6);
    m_sendEdit = new QLineEdit();
    m_sendEdit->setPlaceholderText("输入发送数据...");
    m_sendEdit->setMinimumHeight(34);
    sendRow->addWidget(m_sendEdit, 1);

    m_sendBtn = new QPushButton("发送");
    m_sendBtn->setMinimumHeight(34);
    m_sendBtn->setFixedWidth(60);
    m_sendBtn->setEnabled(false);
    m_sendBtn->setObjectName("successBtn");
    connect(m_sendBtn, &QPushButton::clicked, this, [this]() {
        if (m_sendEdit->text().trimmed().isEmpty()) {
            showToast("请输入要发送的数据");
            return;
        }
        showToast("数据已发送");
        emit serialSendRequested(m_sendEdit->text(), m_hexModeCheck->isChecked());
    });
    sendRow->addWidget(m_sendBtn);

    sendLayout->addLayout(sendRow);
    layout->addWidget(sendGroup);
    layout->addStretch();
}

void SettingsDialog::setupNetworkTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(12);
    layout->setContentsMargins(12, 12, 12, 12);

    // 服务器配置 - 紧凑布局
    QGroupBox *paramGroup = new QGroupBox("服务器配置");
    QGridLayout *grid = new QGridLayout(paramGroup);
    grid->setSpacing(10);
    grid->setContentsMargins(14, 22, 14, 14);

    // 协议选择
    QLabel *protoLabel = new QLabel("协议");
    protoLabel->setFixedWidth(50);
    grid->addWidget(protoLabel, 0, 0);
    m_protocolCombo = new QComboBox();
    m_protocolCombo->addItems({"TCP", "UDP"});
    m_protocolCombo->setMinimumHeight(34);
    grid->addWidget(m_protocolCombo, 0, 1, 1, 3);

    // 服务器地址:端口（同行）
    QLabel *addrLabel = new QLabel("地址");
    addrLabel->setFixedWidth(50);
    grid->addWidget(addrLabel, 1, 0);
    
    QHBoxLayout *addrRow = new QHBoxLayout();
    addrRow->setSpacing(4);
    m_serverAddrEdit = new QLineEdit("127.0.0.1");
    m_serverAddrEdit->setMinimumHeight(34);
    m_serverAddrEdit->setPlaceholderText("IP地址");
    addrRow->addWidget(m_serverAddrEdit, 1);
    
    QLabel *colonLabel = new QLabel(":");
    colonLabel->setStyleSheet("font-weight: bold; color: #64748B;");
    addrRow->addWidget(colonLabel);
    
    m_serverPortSpin = new QSpinBox();
    m_serverPortSpin->setRange(1, 65535);
    m_serverPortSpin->setValue(9000);
    m_serverPortSpin->setMinimumHeight(34);
    m_serverPortSpin->setFixedWidth(80);
    addrRow->addWidget(m_serverPortSpin);
    grid->addLayout(addrRow, 1, 1, 1, 3);

    // 自动上传设置（同行）
    QLabel *uploadLabel = new QLabel("上传");
    uploadLabel->setFixedWidth(50);
    grid->addWidget(uploadLabel, 2, 0);
    
    QHBoxLayout *uploadRow = new QHBoxLayout();
    uploadRow->setSpacing(8);
    m_autoUploadCheck = new QCheckBox("自动");
    m_autoUploadCheck->setChecked(true);
    m_autoUploadCheck->setToolTip("启用自动上传数据");
    uploadRow->addWidget(m_autoUploadCheck);
    
    QLabel *intervalLabel = new QLabel("间隔");
    intervalLabel->setStyleSheet("color: #64748B;");
    uploadRow->addWidget(intervalLabel);
    
    m_uploadIntervalSpin = new QSpinBox();
    m_uploadIntervalSpin->setRange(1000, 60000);
    m_uploadIntervalSpin->setValue(5000);
    m_uploadIntervalSpin->setSuffix(" ms");
    m_uploadIntervalSpin->setMinimumHeight(34);
    m_uploadIntervalSpin->setFixedWidth(100);
    uploadRow->addWidget(m_uploadIntervalSpin);
    uploadRow->addStretch();
    grid->addLayout(uploadRow, 2, 1, 1, 3);

    layout->addWidget(paramGroup);

    // 操作按钮 - 紧凑排列
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    m_netConnectBtn = new QPushButton("连接");
    m_netConnectBtn->setMinimumHeight(38);
    m_netConnectBtn->setObjectName("primaryBtn");
    connect(m_netConnectBtn, &QPushButton::clicked, this, [this]() {
        showToast("正在连接服务器...");
        emit networkConnectRequested();
    });
    btnLayout->addWidget(m_netConnectBtn);

    m_netDisconnectBtn = new QPushButton("断开");
    m_netDisconnectBtn->setMinimumHeight(38);
    m_netDisconnectBtn->setEnabled(false);
    connect(m_netDisconnectBtn, &QPushButton::clicked, this, [this]() {
        showToast("网络已断开");
        emit networkDisconnectRequested();
    });
    btnLayout->addWidget(m_netDisconnectBtn);

    m_netReconnectBtn = new QPushButton("重连");
    m_netReconnectBtn->setMinimumHeight(38);
    m_netReconnectBtn->setEnabled(false);
    m_netReconnectBtn->setObjectName("warningBtn");
    m_netReconnectBtn->setToolTip("手动触发重新连接");
    connect(m_netReconnectBtn, &QPushButton::clicked, this, [this]() {
        showToast("正在重新连接...");
        emit networkReconnectRequested();
    });
    btnLayout->addWidget(m_netReconnectBtn);

    layout->addLayout(btnLayout);

    // 手动上传按钮
    m_manualUploadBtn = new QPushButton("手动上传数据");
    m_manualUploadBtn->setMinimumHeight(38);
    m_manualUploadBtn->setObjectName("purpleBtn");
    m_manualUploadBtn->setToolTip("立即上传当前数据到服务器（需先连接网络）");
    connect(m_manualUploadBtn, &QPushButton::clicked, this, [this]() {
        showToast("正在上传数据...");
        emit manualUploadRequested();
    });
    layout->addWidget(m_manualUploadBtn);

    // 提示信息
    QLabel *noteLabel = new QLabel("提示: 本模块为客户端上传模式，数据将发送至配置的服务器地址");
    noteLabel->setWordWrap(true);
    noteLabel->setObjectName("noteLabel");
    layout->addWidget(noteLabel);

    layout->addStretch();
}

void SettingsDialog::setupCollectTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(12);
    layout->setContentsMargins(12, 12, 12, 12);

    // 采集参数
    QGroupBox *paramGroup = new QGroupBox("采集参数");
    QVBoxLayout *groupLayout = new QVBoxLayout(paramGroup);
    groupLayout->setSpacing(12);
    groupLayout->setContentsMargins(14, 22, 14, 14);

    QHBoxLayout *intervalRow = new QHBoxLayout();
    QLabel *intervalLabel = new QLabel("采集周期");
    intervalLabel->setFixedWidth(60);
    intervalRow->addWidget(intervalLabel);
    
    m_collectIntervalSpin = new QSpinBox();
    m_collectIntervalSpin->setRange(500, 60000);
    m_collectIntervalSpin->setValue(2000);
    m_collectIntervalSpin->setSuffix(" ms");
    m_collectIntervalSpin->setMinimumHeight(34);
    m_collectIntervalSpin->setFixedWidth(120);
    intervalRow->addWidget(m_collectIntervalSpin);
    intervalRow->addStretch();
    groupLayout->addLayout(intervalRow);

    // 提示说明
    QLabel *note = new QLabel("注意: 采集周期仅对模拟模式生效\n真实串口设备由外设主动上报数据，此设置不影响实际采集频率");
    note->setWordWrap(true);
    note->setObjectName("noteLabel");
    groupLayout->addWidget(note);

    layout->addWidget(paramGroup);

    // 应用按钮
    QPushButton *applyBtn = new QPushButton("应用设置");
    applyBtn->setMinimumHeight(38);
    applyBtn->setObjectName("purpleBtn");
    connect(applyBtn, &QPushButton::clicked, this, [this]() {
        showToast("采集设置已应用");
        emit collectIntervalChanged(m_collectIntervalSpin->value());
    });
    layout->addWidget(applyBtn);

    layout->addStretch();
}

void SettingsDialog::applyStyles()
{
    setStyleSheet(R"(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F8FAFC, stop:1 #F1F5F9);
        }
        QTabWidget::pane {
            background: white;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            padding: 6px;
        }
        QTabBar::tab {
            padding: 10px 28px;
            font-size: 13px;
            font-weight: 500;
            color: #64748B;
            background: transparent;
            border: none;
            border-bottom: 2px solid transparent;
            margin-right: 2px;
            min-width: 80px;
        }
        QTabBar::tab:selected {
            color: #3B82F6;
            border-bottom: 2px solid #3B82F6;
            font-weight: 600;
        }
        QTabBar::tab:hover:!selected {
            color: #475569;
            background: rgba(59,130,246,0.05);
            border-radius: 6px 6px 0 0;
        }
        QGroupBox {
            font-weight: 600;
            font-size: 13px;
            color: #1E40AF;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            margin-top: 14px;
            padding-top: 20px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #FFFFFF, stop:1 #FAFBFC);
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 2px 10px;
            background: white;
            border-radius: 4px;
        }
        QLabel {
            font-size: 13px;
            color: #374151;
        }
        QComboBox, QSpinBox, QLineEdit {
            border: 1.5px solid #D1D5DB;
            border-radius: 8px;
            padding: 6px 10px;
            background: white;
            font-size: 13px;
            color: #374151;
            selection-background-color: #DBEAFE;
        }
        QComboBox:hover, QSpinBox:hover, QLineEdit:hover {
            border-color: #9CA3AF;
        }
        QComboBox:focus, QSpinBox:focus, QLineEdit:focus {
            border-color: #3B82F6;
            background: #FAFBFF;
        }
        QComboBox::drop-down {
            border: none;
            width: 28px;
            subcontrol-position: right center;
        }
        QComboBox::down-arrow {
            image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTIiIGhlaWdodD0iOCIgdmlld0JveD0iMCAwIDEyIDgiIGZpbGw9Im5vbmUiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHBhdGggZD0iTTEgMS41TDYgNi41TDExIDEuNSIgc3Ryb2tlPSIjNkI3MjgwIiBzdHJva2Utd2lkdGg9IjIiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIgc3Ryb2tlLWxpbmVqb2luPSJyb3VuZCIvPjwvc3ZnPg==);
            width: 12px;
            height: 8px;
            margin-right: 8px;
        }
        QComboBox QAbstractItemView {
            background: white;
            border: 1px solid #D1D5DB;
            border-radius: 8px;
            selection-background-color: #DBEAFE;
            padding: 4px;
        }
        QComboBox QAbstractItemView::item {
            min-height: 28px;
            padding: 4px 8px;
        }
        QCheckBox {
            font-size: 13px;
            color: #374151;
            spacing: 6px;
        }
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
        }
        QPushButton {
            background: #F3F4F6;
            border: 1px solid #D1D5DB;
            border-radius: 8px;
            padding: 6px 14px;
            font-size: 13px;
            font-weight: 500;
            color: #374151;
        }
        QPushButton:hover {
            background: #E5E7EB;
            border-color: #9CA3AF;
        }
        QPushButton:pressed {
            background: #D1D5DB;
        }
        QPushButton:disabled {
            color: #9CA3AF;
            background: #F9FAFB;
            border-color: #E5E7EB;
        }
        QPushButton#primaryBtn {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #3B82F6, stop:1 #2563EB);
            border: none;
            color: white;
            font-weight: 600;
        }
        QPushButton#primaryBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #60A5FA, stop:1 #3B82F6);
        }
        QPushButton#primaryBtn:pressed {
            background: #1D4ED8;
        }
        QPushButton#primaryBtn:disabled {
            background: #93C5FD;
        }
        QPushButton#successBtn {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #10B981, stop:1 #059669);
            border: none;
            color: white;
            font-weight: 600;
        }
        QPushButton#successBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #34D399, stop:1 #10B981);
        }
        QPushButton#successBtn:pressed {
            background: #047857;
        }
        QPushButton#successBtn:disabled {
            background: #6EE7B7;
        }
        QPushButton#warningBtn {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F59E0B, stop:1 #D97706);
            border: none;
            color: white;
            font-weight: 600;
        }
        QPushButton#warningBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #FBBF24, stop:1 #F59E0B);
        }
        QPushButton#warningBtn:pressed {
            background: #B45309;
        }
        QPushButton#warningBtn:disabled {
            background: #FCD34D;
        }
        QPushButton#purpleBtn {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #8B5CF6, stop:1 #7C3AED);
            border: none;
            color: white;
            font-weight: 600;
        }
        QPushButton#purpleBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #A78BFA, stop:1 #8B5CF6);
        }
        QPushButton#purpleBtn:pressed {
            background: #6D28D9;
        }
        QLabel#noteLabel {
            color: #92400E;
            font-size: 12px;
            padding: 10px 12px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #FEF3C7, stop:1 #FDE68A);
            border-radius: 8px;
            border: 1px solid #FCD34D;
            line-height: 1.4;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            width: 20px;
            border: none;
            background: transparent;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background: #F1F5F9;
        }
        QSpinBox::up-arrow {
            image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTAiIGhlaWdodD0iNiIgdmlld0JveD0iMCAwIDEwIDYiIGZpbGw9Im5vbmUiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHBhdGggZD0iTTEgNUw1IDFMOSA1IiBzdHJva2U9IiM2QjcyODAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiLz48L3N2Zz4=);
            width: 10px;
            height: 6px;
        }
        QSpinBox::down-arrow {
            image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTAiIGhlaWdodD0iNiIgdmlld0JveD0iMCAwIDEwIDYiIGZpbGw9Im5vbmUiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHBhdGggZD0iTTEgMUw1IDVMOSA1IiBzdHJva2U9IiM2QjcyODAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiLz48L3N2Zz4=);
            width: 10px;
            height: 6px;
        }
        QToolTip {
            background: #1E293B;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 6px 10px;
            font-size: 12px;
        }
    )");
}

// Getters
QString SettingsDialog::portName() const { return m_portCombo->currentText().split(" ").first(); }
int SettingsDialog::baudRate() const { return m_baudCombo->currentText().toInt(); }
int SettingsDialog::dataBits() const { return m_dataBitsCombo->currentText().toInt(); }
int SettingsDialog::parityIndex() const { return m_parityCombo->currentIndex(); }
int SettingsDialog::stopBitsIndex() const { return m_stopBitsCombo->currentIndex(); }
int SettingsDialog::flowControlIndex() const { return m_flowCombo->currentIndex(); }
QString SettingsDialog::protocol() const { return m_protocolCombo->currentText(); }
QString SettingsDialog::serverAddress() const { return m_serverAddrEdit->text(); }
int SettingsDialog::serverPort() const { return m_serverPortSpin->value(); }
bool SettingsDialog::autoUpload() const { return m_autoUploadCheck->isChecked(); }
int SettingsDialog::uploadInterval() const { return m_uploadIntervalSpin->value(); }
int SettingsDialog::collectInterval() const { return m_collectIntervalSpin->value(); }

// Setters
void SettingsDialog::setPortList(const QStringList &ports) {
    m_portCombo->clear();
    m_portCombo->addItems(ports);
}
void SettingsDialog::setBaudRate(int rate) { m_baudCombo->setCurrentText(QString::number(rate)); }
void SettingsDialog::setServerAddress(const QString &addr) { m_serverAddrEdit->setText(addr); }
void SettingsDialog::setServerPort(int port) { m_serverPortSpin->setValue(port); }
void SettingsDialog::setProtocol(const QString &proto) { m_protocolCombo->setCurrentText(proto); }
void SettingsDialog::setAutoUpload(bool enabled) { m_autoUploadCheck->setChecked(enabled); }
void SettingsDialog::setUploadInterval(int ms) { m_uploadIntervalSpin->setValue(ms); }
void SettingsDialog::setCollectInterval(int ms) { m_collectIntervalSpin->setValue(ms); }

// Slots
void SettingsDialog::setSerialConnected(bool connected) {
    m_serialConnectBtn->setEnabled(!connected);
    m_serialDisconnectBtn->setEnabled(connected);
    m_sendBtn->setEnabled(connected);
}

void SettingsDialog::setNetworkConnected(bool connected) {
    m_netConnectBtn->setEnabled(!connected);
    m_netDisconnectBtn->setEnabled(connected);
    m_netReconnectBtn->setEnabled(!connected);
}

void SettingsDialog::setSimulating(bool simulating) {
    m_simulateBtn->setText(simulating ? "停止模拟" : "模拟");
}

void SettingsDialog::showToast(const QString &message, int duration)
{
    m_toastLabel->setText(message);
    m_toastLabel->adjustSize();
    
    // 居中显示
    int x = (width() - m_toastLabel->width()) / 2;
    int y = height() - m_toastLabel->height() - 60;
    m_toastLabel->move(x, y);
    m_toastLabel->raise();
    m_toastLabel->show();
    
    m_toastTimer->start(duration);
}
