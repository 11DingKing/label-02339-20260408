#include "mainwindow.h"
#include "dialogs/settingsdialog.h"
#include "utils/logmanager.h"
#include "utils/stringutils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QGroupBox>
#include <QFrame>
#include <QMessageBox>
#include <QFileDialog>
#include <QCheckBox>
#include <QHeaderView>
#include <QApplication>
#include <QDebug>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settingsDialog(nullptr)
{
    setWindowTitle("环境监测系统 - EnvMonitor");
    setMinimumSize(1100, 800);
    resize(1280, 900);

    // 初始化日志管理器
    LogManager::instance()->initialize();
    LogManager::instance()->info("MainWindow", "环境监测系统启动");

    // 初始化管理器
    m_serialMgr = new SerialManager(this);
    m_networkMgr = new NetworkManager(this);
    m_dbMgr = new DatabaseManager(this);
    m_configMgr = new ConfigManager(this);
    m_dataProcessor = new DataProcessor(this);

    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    applyStyleSheet();
    initManagers();
    connectSignals();
    loadConfigToUI();
}

MainWindow::~MainWindow()
{
    LogManager::instance()->info("MainWindow", "环境监测系统关闭");
    LogManager::instance()->close();
}

// ==================== UI 构建 ====================

void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(16, 8, 16, 8);
    mainLayout->setSpacing(12);

    // 上半部分：实时数据监测（占更大空间）
    mainLayout->addWidget(createRealtimePanel(), 2);

    // 下半部分：历史数据 + 日志
    QSplitter *bottomSplitter = new QSplitter(Qt::Horizontal);
    bottomSplitter->addWidget(createHistoryPanel());
    bottomSplitter->addWidget(createLogPanel());
    bottomSplitter->setStretchFactor(0, 3);
    bottomSplitter->setStretchFactor(1, 2);

    mainLayout->addWidget(bottomSplitter, 1);
    setCentralWidget(central);
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("文件(&F)");
    fileMenu->addAction("加载配置(&L)", QKeySequence("Ctrl+O"), this, &MainWindow::onLoadConfig);
    fileMenu->addAction("保存配置(&S)", QKeySequence("Ctrl+S"), this, &MainWindow::onSaveConfig);
    fileMenu->addSeparator();
    fileMenu->addAction("退出(&Q)", QKeySequence("Ctrl+Q"), this, &QWidget::close);

    QMenu *settingsMenu = menuBar()->addMenu("设置(&E)");
    settingsMenu->addAction("打开设置", this, &MainWindow::showSettingsDialog);
    settingsMenu->addAction("刷新串口列表", this, [this]() {
        if (m_settingsDialog) {
            m_settingsDialog->setPortList(SerialManager::availablePorts());
        }
        showMessage("串口列表已刷新");
    });

    QMenu *helpMenu = menuBar()->addMenu("帮助(&H)");
    helpMenu->addAction("关于(&A)", this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolbar = addToolBar("工具栏");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(20, 20));

    toolbar->addAction("设置", this, &MainWindow::showSettingsDialog);
    toolbar->addSeparator();
    toolbar->addAction("一键模拟", this, &MainWindow::onQuickSimulate);
    toolbar->addSeparator();
    toolbar->addAction("查询历史", this, &MainWindow::onQueryHistory);
}

void MainWindow::setupStatusBar()
{
    m_serialStatusLabel = new QLabel("串口: 未连接");
    m_networkStatusLabel = new QLabel("网络: 未连接");
    m_dbStatusLabel = new QLabel("数据库: 未连接");

    statusBar()->addWidget(m_serialStatusLabel, 1);
    statusBar()->addWidget(m_networkStatusLabel, 1);
    statusBar()->addWidget(m_dbStatusLabel, 1);
}

// ==================== 面板创建 ====================

QWidget* MainWindow::createRealtimePanel()
{
    QGroupBox *group = new QGroupBox("📊 实时数据监测");
    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setSpacing(12);

    // 仪表盘区域
    QHBoxLayout *gaugeLayout = new QHBoxLayout();
    gaugeLayout->setSpacing(16);

    // 温度仪表盘
    QVBoxLayout *tempLayout = new QVBoxLayout();
    m_tempGauge = new GaugeWidget();
    m_tempGauge->setTitle("温度");
    m_tempGauge->setUnit("℃");
    m_tempGauge->setMinValue(-20);
    m_tempGauge->setMaxValue(60);
    m_tempGauge->setArcColor(QColor("#EF4444"));
    m_tempBar = new QProgressBar();
    m_tempBar->setRange(-20, 60);
    m_tempBar->setTextVisible(true);
    m_tempBar->setFormat("%v ℃");
    m_tempBar->setStyleSheet("QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #EF4444, stop:1 #F87171); }");
    m_tempValueLabel = new QLabel("-- ℃");
    m_tempValueLabel->setAlignment(Qt::AlignCenter);
    m_tempValueLabel->setObjectName("valueLabel");
    tempLayout->addWidget(m_tempGauge);
    tempLayout->addWidget(m_tempBar);
    tempLayout->addWidget(m_tempValueLabel);

    // 湿度仪表盘
    QVBoxLayout *humLayout = new QVBoxLayout();
    m_humGauge = new GaugeWidget();
    m_humGauge->setTitle("湿度");
    m_humGauge->setUnit("%");
    m_humGauge->setMinValue(0);
    m_humGauge->setMaxValue(100);
    m_humGauge->setArcColor(QColor("#3B82F6"));
    m_humBar = new QProgressBar();
    m_humBar->setRange(0, 100);
    m_humBar->setTextVisible(true);
    m_humBar->setFormat("%v %");
    m_humBar->setStyleSheet("QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3B82F6, stop:1 #60A5FA); }");
    m_humValueLabel = new QLabel("-- %");
    m_humValueLabel->setAlignment(Qt::AlignCenter);
    m_humValueLabel->setObjectName("valueLabel");
    humLayout->addWidget(m_humGauge);
    humLayout->addWidget(m_humBar);
    humLayout->addWidget(m_humValueLabel);

    // 光照仪表盘
    QVBoxLayout *luxLayout = new QVBoxLayout();
    m_luxGauge = new GaugeWidget();
    m_luxGauge->setTitle("光照强度");
    m_luxGauge->setUnit("Lux");
    m_luxGauge->setMinValue(0);
    m_luxGauge->setMaxValue(5000);
    m_luxGauge->setArcColor(QColor("#F59E0B"));
    m_luxBar = new QProgressBar();
    m_luxBar->setRange(0, 5000);
    m_luxBar->setTextVisible(true);
    m_luxBar->setFormat("%v Lux");
    m_luxBar->setStyleSheet("QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #F59E0B, stop:1 #FBBF24); }");
    m_luxValueLabel = new QLabel("-- Lux");
    m_luxValueLabel->setAlignment(Qt::AlignCenter);
    m_luxValueLabel->setObjectName("valueLabel");
    luxLayout->addWidget(m_luxGauge);
    luxLayout->addWidget(m_luxBar);
    luxLayout->addWidget(m_luxValueLabel);

    gaugeLayout->addLayout(tempLayout);
    gaugeLayout->addLayout(humLayout);
    gaugeLayout->addLayout(luxLayout);

    layout->addLayout(gaugeLayout);

    // 最后更新时间
    m_lastUpdateLabel = new QLabel("最后更新: --");
    m_lastUpdateLabel->setAlignment(Qt::AlignRight);
    m_lastUpdateLabel->setStyleSheet("color: #64748B; font-size: 12px; font-weight: 500; padding: 4px 8px; background: rgba(241,245,249,0.8); border-radius: 6px;");
    layout->addWidget(m_lastUpdateLabel);

    return group;
}

QWidget* MainWindow::createHistoryPanel()
{
    QGroupBox *group = new QGroupBox("📋 历史数据查询");
    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setSpacing(16);
    layout->setContentsMargins(16, 24, 16, 16);

    // 查询条件 - 卡片式设计
    QFrame *queryFrame = new QFrame();
    queryFrame->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(248,250,252,0.95), stop:1 rgba(241,245,249,0.9));
            border-radius: 14px;
            padding: 4px;
        }
    )");
    QHBoxLayout *queryLayout = new QHBoxLayout(queryFrame);
    queryLayout->setSpacing(12);
    queryLayout->setContentsMargins(16, 14, 16, 14);

    // 起始时间
    QLabel *fromLabel = new QLabel("起始");
    fromLabel->setStyleSheet("font-weight: 600; color: #475569; font-size: 13px;");
    queryLayout->addWidget(fromLabel);
    
    m_queryFrom = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-1));
    m_queryFrom->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_queryFrom->setCalendarPopup(true);
    m_queryFrom->setMinimumHeight(40);
    m_queryFrom->setMinimumWidth(170);
    m_queryFrom->setStyleSheet(R"(
        QDateTimeEdit {
            border: 2px solid #E2E8F0;
            border-radius: 10px;
            padding: 8px 12px;
            background: white;
            font-size: 13px;
            color: #334155;
            font-weight: 500;
        }
        QDateTimeEdit:hover { border-color: #CBD5E1; }
        QDateTimeEdit:focus { border-color: #3B82F6; background: #FAFBFF; }
        QDateTimeEdit::drop-down {
            border: none;
            width: 28px;
            background: transparent;
            subcontrol-position: right center;
        }
        QDateTimeEdit::down-arrow {
            image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTIiIGhlaWdodD0iOCIgdmlld0JveD0iMCAwIDEyIDgiIGZpbGw9Im5vbmUiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHBhdGggZD0iTTEgMS41TDYgNi41TDExIDEuNSIgc3Ryb2tlPSIjNjQ3NDhCIiBzdHJva2Utd2lkdGg9IjIiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIgc3Ryb2tlLWxpbmVqb2luPSJyb3VuZCIvPjwvc3ZnPg==);
            width: 12px;
            height: 8px;
        }
        QCalendarWidget {
            background: white;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
        }
        QCalendarWidget QToolButton {
            color: #334155;
            background: transparent;
            border: none;
            border-radius: 6px;
            padding: 6px;
            font-weight: 600;
        }
        QCalendarWidget QToolButton:hover {
            background: #DBEAFE;
            color: #1E40AF;
        }
        QCalendarWidget QMenu {
            background: white;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
        }
        QCalendarWidget QSpinBox {
            border: 1px solid #E2E8F0;
            border-radius: 6px;
            padding: 4px;
            background: white;
        }
        QCalendarWidget QAbstractItemView:enabled {
            background: white;
            color: #334155;
            selection-background-color: #3B82F6;
            selection-color: white;
        }
        QCalendarWidget QAbstractItemView:disabled {
            color: #CBD5E1;
        }
        QCalendarWidget QWidget#qt_calendar_navigationbar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F8FAFC, stop:1 #F1F5F9);
            border-bottom: 1px solid #E2E8F0;
            border-radius: 12px 12px 0 0;
            padding: 4px;
        }
    )");
    queryLayout->addWidget(m_queryFrom);

    // 分隔符
    QLabel *toArrow = new QLabel("→");
    toArrow->setStyleSheet("color: #94A3B8; font-size: 16px; font-weight: bold;");
    queryLayout->addWidget(toArrow);

    // 结束时间
    QLabel *toLabel = new QLabel("结束");
    toLabel->setStyleSheet("font-weight: 600; color: #475569; font-size: 13px;");
    queryLayout->addWidget(toLabel);
    
    m_queryTo = new QDateTimeEdit(QDateTime::currentDateTime());
    m_queryTo->setDisplayFormat("yyyy-MM-dd HH:mm");
    m_queryTo->setCalendarPopup(true);
    m_queryTo->setMinimumHeight(40);
    m_queryTo->setMinimumWidth(170);
    m_queryTo->setStyleSheet(m_queryFrom->styleSheet());
    queryLayout->addWidget(m_queryTo);

    queryLayout->addSpacing(8);

    // 查询按钮
    QPushButton *queryBtn = new QPushButton("🔍 查询");
    queryBtn->setObjectName("primaryBtn");
    queryBtn->setMinimumHeight(40);
    queryBtn->setMinimumWidth(90);
    queryBtn->setCursor(Qt::PointingHandCursor);
    queryBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #3B82F6, stop:1 #2563EB);
            color: white;
            border: none;
            border-radius: 10px;
            font-weight: 600;
            font-size: 13px;
            padding: 0 16px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #60A5FA, stop:1 #3B82F6);
        }
        QPushButton:pressed { background: #1D4ED8; }
    )");
    connect(queryBtn, &QPushButton::clicked, this, &MainWindow::onQueryHistory);
    queryLayout->addWidget(queryBtn);

    // 删除按钮
    QPushButton *deleteBtn = new QPushButton("🗑 删除");
    deleteBtn->setObjectName("dangerBtn");
    deleteBtn->setMinimumHeight(40);
    deleteBtn->setMinimumWidth(90);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #EF4444, stop:1 #DC2626);
            color: white;
            border: none;
            border-radius: 10px;
            font-weight: 600;
            font-size: 13px;
            padding: 0 16px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F87171, stop:1 #EF4444);
        }
        QPushButton:pressed { background: #B91C1C; }
    )");
    connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteHistory);
    queryLayout->addWidget(deleteBtn);

    queryLayout->addStretch();

    // 记录数标签
    m_recordCountLabel = new QLabel("共 0 条记录");
    m_recordCountLabel->setStyleSheet(R"(
        QLabel {
            color: #64748B;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 14px;
            background: white;
            border-radius: 8px;
            border: 1px solid #E2E8F0;
        }
    )");
    queryLayout->addWidget(m_recordCountLabel);

    layout->addWidget(queryFrame);

    // 数据表格 - 现代化样式
    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(5);
    m_historyTable->setHorizontalHeaderLabels({"ID", "采集时间", "温度(℃)", "湿度(%)", "光照(Lux)"});
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setShowGrid(false);
    m_historyTable->verticalHeader()->setVisible(false);
    m_historyTable->setMinimumHeight(320);  // 至少显示10条数据
    m_historyTable->setStyleSheet(R"(
        QTableWidget {
            background: white;
            border: none;
            border-radius: 14px;
            font-size: 13px;
            selection-background-color: #DBEAFE;
            selection-color: #1E40AF;
        }
        QTableWidget::item {
            padding: 14px 12px;
            border-bottom: 1px solid #F1F5F9;
        }
        QTableWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #DBEAFE, stop:1 #BFDBFE);
            color: #1E40AF;
        }
        QTableWidget::item:hover {
            background: #F8FAFC;
        }
        QTableWidget::item:alternate {
            background: #FAFBFC;
        }
        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F8FAFC, stop:1 #F1F5F9);
            border: none;
            border-bottom: 2px solid #3B82F6;
            padding: 14px 12px;
            font-weight: 700;
            font-size: 12px;
            color: #334155;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        QHeaderView::section:first {
            border-top-left-radius: 14px;
        }
        QHeaderView::section:last {
            border-top-right-radius: 14px;
        }
        QScrollBar:vertical {
            background: #F8FAFC;
            width: 10px;
            margin: 4px 2px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical {
            background: #CBD5E1;
            border-radius: 5px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #94A3B8;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
    )");
    layout->addWidget(m_historyTable);

    return group;
}

QWidget* MainWindow::createLogPanel()
{
    QGroupBox *group = new QGroupBox("运行日志");
    QVBoxLayout *layout = new QVBoxLayout(group);

    m_logView = new QTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setStyleSheet(R"(
        QTextEdit {
            font-family: 'SF Mono', 'Menlo', 'Monaco', 'Courier New', monospace;
            font-size: 12px;
            background: #1E293B;
            color: #E2E8F0;
            border: none;
            border-radius: 12px;
            padding: 12px;
        }
    )");
    layout->addWidget(m_logView);

    QPushButton *clearLogBtn = new QPushButton("清空日志");
    clearLogBtn->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: 1px solid #64748B;
            color: #64748B;
            border-radius: 8px;
            padding: 8px 16px;
        }
        QPushButton:hover {
            background: rgba(100, 116, 139, 0.1);
            border-color: #94A3B8;
            color: #94A3B8;
        }
    )");
    connect(clearLogBtn, &QPushButton::clicked, m_logView, &QTextEdit::clear);
    layout->addWidget(clearLogBtn);

    return group;
}

void MainWindow::showSettingsDialog()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
        
        // 连接信号
        connect(m_settingsDialog, &SettingsDialog::refreshPortsRequested, this, [this]() {
            m_settingsDialog->setPortList(SerialManager::availablePorts());
        });
        connect(m_settingsDialog, &SettingsDialog::serialConnectRequested, this, &MainWindow::onConnectSerial);
        connect(m_settingsDialog, &SettingsDialog::serialDisconnectRequested, this, &MainWindow::onDisconnectSerial);
        connect(m_settingsDialog, &SettingsDialog::simulateRequested, this, &MainWindow::onStartSimulation);
        connect(m_settingsDialog, &SettingsDialog::serialSendRequested, this, &MainWindow::onSerialSend);
        connect(m_settingsDialog, &SettingsDialog::networkConnectRequested, this, &MainWindow::onConnectNetwork);
        connect(m_settingsDialog, &SettingsDialog::networkDisconnectRequested, this, &MainWindow::onDisconnectNetwork);
        connect(m_settingsDialog, &SettingsDialog::networkReconnectRequested, this, &MainWindow::onResetReconnect);
        connect(m_settingsDialog, &SettingsDialog::manualUploadRequested, this, &MainWindow::onManualUpload);
        connect(m_settingsDialog, &SettingsDialog::collectIntervalChanged, this, &MainWindow::onCollectIntervalChanged);
        
        // 初始化数据
        m_settingsDialog->setPortList(SerialManager::availablePorts());
        m_settingsDialog->setBaudRate(m_configMgr->serialBaudRate());
        m_settingsDialog->setServerAddress(m_configMgr->serverAddress());
        m_settingsDialog->setServerPort(m_configMgr->serverPort());
        m_settingsDialog->setProtocol(m_configMgr->networkProtocol());
        m_settingsDialog->setAutoUpload(m_configMgr->autoUpload());
        m_settingsDialog->setUploadInterval(m_configMgr->uploadInterval());
        m_settingsDialog->setCollectInterval(m_configMgr->collectInterval());
    }
    
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void MainWindow::onCollectIntervalChanged(int ms)
{
    m_configMgr->setCollectInterval(ms);
    if (m_serialMgr->isSimulating()) {
        m_serialMgr->stopSimulation();
        m_serialMgr->startSimulation(ms);
    }
    showMessage(QString("采集周期已更新为 %1 ms").arg(ms));
}

void MainWindow::onQuickSimulate()
{
    // 一键模拟：启动串口模拟 + 网络连接
    appendLog("[一键模拟] 启动质检模式...");
    
    // 检查是否已在模拟，如果是则停止
    if (m_serialMgr->isSimulating()) {
        m_serialMgr->stopSimulation();
        if (m_settingsDialog) {
            m_settingsDialog->setSimulating(false);
        }
        m_networkMgr->disconnectFromServer();
        appendLog("[一键模拟] 模拟已停止");
        showMessage("一键模拟已停止");
        return;
    }
    
    // 1. 启动串口模拟
    int interval = m_configMgr->collectInterval();
    m_serialMgr->startSimulation(interval);
    
    // 确保设置对话框存在并同步状态
    if (!m_settingsDialog) {
        showSettingsDialog();  // 创建对话框
        m_settingsDialog->hide();  // 先隐藏
    }
    m_settingsDialog->setSimulating(true);
    
    appendLog("[一键模拟] 串口模拟已启动");
    
    // 2. 连接网络（使用配置中的地址，默认 127.0.0.1:9000）
    QString addr = m_configMgr->serverAddress();
    quint16 port = m_configMgr->serverPort();
    QString proto = m_configMgr->networkProtocol();
    
    NetworkManager::Protocol protocol = (proto == "TCP") 
        ? NetworkManager::TCP : NetworkManager::UDP;
    
    m_networkMgr->connectToServer(addr, port, protocol);
    
    // 启用自动上传
    if (m_configMgr->autoUpload()) {
        m_networkMgr->setAutoUpload(true, m_configMgr->uploadInterval());
    }
    
    appendLog(QString("[一键模拟] 网络连接中: %1:%2 (%3)").arg(addr).arg(port).arg(proto));
    showMessage("一键模拟已启动：串口模拟 + 网络上传");
}

// ==================== 初始化与信号连接 ====================

void MainWindow::initManagers()
{
    // 加载配置
    m_configMgr->loadConfig();

    // 初始化数据库
    QString dbPath = m_configMgr->databasePath();
    if (m_dbMgr->initialize(dbPath)) {
        m_dbStatusLabel->setText("数据库: ✅ 已连接");
        m_dbStatusLabel->setStyleSheet("color: #4CAF50;");
        appendLog("数据库初始化成功: " + dbPath);
    } else {
        m_dbStatusLabel->setText("数据库: ❌ 连接失败");
        m_dbStatusLabel->setStyleSheet("color: #F44336;");
    }

    // 设置数据处理器
    m_dataProcessor->setDatabaseManager(m_dbMgr);
    m_dataProcessor->setNetworkManager(m_networkMgr);
}

void MainWindow::connectSignals()
{
    // 串口 -> 数据处理器
    connect(m_serialMgr, &SerialManager::dataReceived,
            m_dataProcessor, &DataProcessor::processIncomingData);

    // 数据处理器 -> UI 更新
    connect(m_dataProcessor, &DataProcessor::dataUpdated,
            this, &MainWindow::onDataUpdated);

    // 串口状态
    connect(m_serialMgr, &SerialManager::connectionStatusChanged,
            this, &MainWindow::onSerialStatusChanged);
    connect(m_serialMgr, &SerialManager::errorOccurred,
            this, &MainWindow::onSerialError);
    connect(m_serialMgr, &SerialManager::portDisconnected,
            this, &MainWindow::onSerialDisconnected);
    connect(m_serialMgr, &SerialManager::rawDataReceived, this, [this](const QByteArray &raw) {
        appendLog("[串口接收] " + QString::fromUtf8(raw));
    });

    // 网络状态
    connect(m_networkMgr, &NetworkManager::connectionStatusChanged,
            this, &MainWindow::onNetworkStatusChanged);
    connect(m_networkMgr, &NetworkManager::networkError,
            this, &MainWindow::onNetworkError);
    connect(m_networkMgr, &NetworkManager::uploadSuccess,
            this, &MainWindow::onUploadSuccess);
    connect(m_networkMgr, &NetworkManager::uploadFailed,
            this, &MainWindow::onUploadFailed);

    // 数据库错误
    connect(m_dbMgr, &DatabaseManager::databaseError,
            this, &MainWindow::onDatabaseError);
}

void MainWindow::loadConfigToUI()
{
    // 配置加载到设置对话框（如果已创建）
    if (m_settingsDialog) {
        m_settingsDialog->setBaudRate(m_configMgr->serialBaudRate());
        m_settingsDialog->setServerAddress(m_configMgr->serverAddress());
        m_settingsDialog->setServerPort(m_configMgr->serverPort());
        m_settingsDialog->setProtocol(m_configMgr->networkProtocol());
        m_settingsDialog->setAutoUpload(m_configMgr->autoUpload());
        m_settingsDialog->setUploadInterval(m_configMgr->uploadInterval());
        m_settingsDialog->setCollectInterval(m_configMgr->collectInterval());
    }
}

// ==================== 槽函数实现 ====================

void MainWindow::onDataUpdated(const SensorData &data)
{
    // 更新仪表盘
    m_tempGauge->setValue(data.temperature());
    m_humGauge->setValue(data.humidity());
    m_luxGauge->setValue(data.lightIntensity());

    // 更新进度条
    m_tempBar->setValue(static_cast<int>(data.temperature()));
    m_humBar->setValue(static_cast<int>(data.humidity()));
    m_luxBar->setValue(static_cast<int>(data.lightIntensity()));

    // 更新数值标签
    m_tempValueLabel->setText(QString("%1 ℃").arg(data.temperature(), 0, 'f', 1));
    m_humValueLabel->setText(QString("%1 %").arg(data.humidity(), 0, 'f', 1));
    m_luxValueLabel->setText(QString("%1 Lux").arg(data.lightIntensity(), 0, 'f', 0));

    // 更新时间
    m_lastUpdateLabel->setText("最后更新: " + data.collectTime().toString("yyyy-MM-dd HH:mm:ss"));
}

void MainWindow::onConnectSerial()
{
    if (!m_settingsDialog) {
        showMessage("请先打开设置对话框", true);
        return;
    }
    
    QString portName = m_settingsDialog->portName();
    if (portName.isEmpty()) {
        showMessage("请选择串口", true);
        return;
    }

    qint32 baudRate = m_settingsDialog->baudRate();

    // 数据位映射
    QSerialPort::DataBits dataBits = static_cast<QSerialPort::DataBits>(
        m_settingsDialog->dataBits());

    // 校验位映射
    QSerialPort::Parity parityMap[] = {
        QSerialPort::NoParity, QSerialPort::EvenParity,
        QSerialPort::OddParity, QSerialPort::SpaceParity, QSerialPort::MarkParity
    };
    QSerialPort::Parity parity = parityMap[m_settingsDialog->parityIndex()];

    // 停止位映射
    QSerialPort::StopBits stopMap[] = {
        QSerialPort::OneStop, QSerialPort::OneAndHalfStop, QSerialPort::TwoStop
    };
    QSerialPort::StopBits stopBits = stopMap[m_settingsDialog->stopBitsIndex()];

    // 流控映射
    QSerialPort::FlowControl flowMap[] = {
        QSerialPort::NoFlowControl, QSerialPort::HardwareControl, QSerialPort::SoftwareControl
    };
    QSerialPort::FlowControl flow = flowMap[m_settingsDialog->flowControlIndex()];

    if (m_serialMgr->openPort(portName, baudRate, dataBits, parity, stopBits, flow)) {
        showMessage("串口连接成功: " + portName);
        appendLog("[串口] 已连接: " + portName + " @ " + QString::number(baudRate));
    }
}

void MainWindow::onDisconnectSerial()
{
    m_serialMgr->closePort();
    m_serialMgr->stopSimulation();
    showMessage("串口已断开");
    appendLog("[串口] 已断开连接");
}

void MainWindow::onStartSimulation()
{
    if (m_serialMgr->isSimulating()) {
        m_serialMgr->stopSimulation();
        if (m_settingsDialog) m_settingsDialog->setSimulating(false);
        showMessage("模拟模式已停止");
        appendLog("[串口] 模拟模式已停止");
    } else {
        int interval = m_settingsDialog ? m_settingsDialog->collectInterval() : 2000;
        m_serialMgr->startSimulation(interval);
        if (m_settingsDialog) m_settingsDialog->setSimulating(true);
        showMessage("模拟模式已启动");
        appendLog("[串口] 模拟模式已启动, 间隔: " + QString::number(interval) + "ms");
    }
}

void MainWindow::onSerialStatusChanged(bool connected)
{
    if (m_settingsDialog) m_settingsDialog->setSerialConnected(connected);
    m_serialStatusLabel->setText(connected ? "串口: 已连接" : "串口: 未连接");
    m_serialStatusLabel->setStyleSheet(connected ? "color: #4CAF50;" : "color: #F44336;");
}

void MainWindow::onSerialDisconnected()
{
    showMessage("串口设备已断开，请检查连接并重新连接", true);
    appendLog("[串口] 设备断开连接，请检查设备并重新连接");
    statusBar()->showMessage("⚠️ 串口断开，请检查设备连接后点击「连接」重试", 10000);
    LogManager::instance()->warning("Serial", "串口设备意外断开");
}

void MainWindow::onSerialSend(const QString &data, bool hexMode)
{
    QString text = data.trimmed();
    if (text.isEmpty()) {
        showMessage("请输入要发送的数据", true);
        return;
    }

    QByteArray sendData;
    if (hexMode) {
        // 十六进制模式：解析如 "AA BB CC" 或 "AABBCC"
        QString hex = text;
        hex.remove(QRegularExpression("[^0-9A-Fa-f]"));
        if (hex.length() % 2 != 0) {
            showMessage("十六进制格式错误：字节数不完整", true);
            return;
        }
        for (int i = 0; i + 1 < hex.length(); i += 2) {
            bool ok;
            sendData.append(static_cast<char>(hex.mid(i, 2).toInt(&ok, 16)));
        }
    } else {
        // 文本模式
        sendData = text.toUtf8();
    }

    if (m_serialMgr->sendData(sendData)) {
        QString logMsg = hexMode 
            ? QString("[串口发送] HEX: %1").arg(sendData.toHex(' ').toUpper().constData())
            : QString("[串口发送] %1").arg(text);
        appendLog(logMsg);
        showMessage(QString("已发送 %1 字节").arg(sendData.size()));
    }
}

void MainWindow::onConnectNetwork()
{
    if (!m_settingsDialog) {
        showMessage("请先打开设置对话框", true);
        return;
    }
    
    QString addr = m_settingsDialog->serverAddress().trimmed();
    if (addr.isEmpty()) {
        showMessage("请输入服务器地址", true);
        return;
    }

    // 使用工具类验证地址格式
    if (!StringUtils::isValidIpAddress(addr)) {
        showMessage("服务器地址格式无效，请输入有效的 IP 地址", true);
        return;
    }

    int portValue = m_settingsDialog->serverPort();
    if (!StringUtils::isValidPort(portValue)) {
        showMessage("端口号无效，请输入 1-65535 范围内的端口", true);
        return;
    }

    quint16 port = static_cast<quint16>(portValue);
    NetworkManager::Protocol proto = m_settingsDialog->protocol() == "TCP"
                                         ? NetworkManager::TCP : NetworkManager::UDP;

    m_networkMgr->connectToServer(addr, port, proto);

    // 设置自动上传
    if (m_settingsDialog->autoUpload()) {
        m_networkMgr->setAutoUpload(true, m_settingsDialog->uploadInterval());
    }

    appendLog("[网络] 正在连接: " + addr + ":" + QString::number(port));
}

void MainWindow::onDisconnectNetwork()
{
    m_networkMgr->disconnectFromServer();
    showMessage("网络已断开");
    appendLog("[网络] 已断开连接");
}

void MainWindow::onManualUpload()
{
    if (!m_networkMgr->isConnected()) {
        showMessage("请先连接网络后再上传数据", true);
        return;
    }
    m_networkMgr->manualUpload();
    appendLog("[网络] 手动上传触发");
}

void MainWindow::onNetworkStatusChanged(bool connected)
{
    if (m_settingsDialog) m_settingsDialog->setNetworkConnected(connected);
    m_networkStatusLabel->setText(connected ? "网络: 已连接" : "网络: 未连接");
    m_networkStatusLabel->setStyleSheet(connected ? "color: #4CAF50;" : "color: #F44336;");

    if (!connected) {
        statusBar()->showMessage("网络连接中断，正在自动重连...", 5000);
    }
}

void MainWindow::onResetReconnect()
{
    m_networkMgr->resetReconnect();
    showMessage("已重置重连，正在尝试连接...");
    appendLog("[网络] 手动触发重连");
}

void MainWindow::onQueryHistory()
{
    QDateTime from = m_queryFrom->dateTime();
    QDateTime to = m_queryTo->dateTime();

    if (from >= to) {
        showMessage("起始时间必须早于结束时间", true);
        return;
    }

    QList<SensorData> records = m_dbMgr->queryByTimeRange(from, to);

    m_historyTable->setRowCount(records.size());
    for (int i = 0; i < records.size(); ++i) {
        const SensorData &d = records[i];
        m_historyTable->setItem(i, 0, new QTableWidgetItem(QString::number(d.id())));
        m_historyTable->setItem(i, 1, new QTableWidgetItem(d.collectTime().toString("yyyy-MM-dd HH:mm:ss")));
        m_historyTable->setItem(i, 2, new QTableWidgetItem(QString::number(d.temperature(), 'f', 1)));
        m_historyTable->setItem(i, 3, new QTableWidgetItem(QString::number(d.humidity(), 'f', 1)));
        m_historyTable->setItem(i, 4, new QTableWidgetItem(QString::number(d.lightIntensity(), 'f', 0)));
    }

    m_recordCountLabel->setText(QString("共 %1 条记录").arg(records.size()));
    showMessage(QString("查询完成，共 %1 条记录").arg(records.size()));
    appendLog(QString("[数据库] 查询历史: %1 ~ %2, 结果: %3 条")
                  .arg(from.toString("yyyy-MM-dd HH:mm"))
                  .arg(to.toString("yyyy-MM-dd HH:mm"))
                  .arg(records.size()));
}

void MainWindow::onDeleteHistory()
{
    QDateTime from = m_queryFrom->dateTime();
    QDateTime to = m_queryTo->dateTime();

    if (from >= to) {
        showMessage("起始时间必须早于结束时间", true);
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认删除");
    msgBox.setText(QString("确定要删除 %1 至 %2 的历史数据吗？\n此操作不可撤销。")
            .arg(from.toString("yyyy-MM-dd HH:mm"))
            .arg(to.toString("yyyy-MM-dd HH:mm")));
    msgBox.setIcon(QMessageBox::Question);
    QPushButton *yesBtn = msgBox.addButton("确定", QMessageBox::YesRole);
    msgBox.addButton("取消", QMessageBox::NoRole);
    msgBox.exec();

    if (msgBox.clickedButton() == yesBtn) {
        int deleted = m_dbMgr->deleteByTimeRange(from, to);
        if (deleted >= 0) {
            showMessage(QString("已删除 %1 条记录").arg(deleted));
            appendLog(QString("[数据库] 删除记录: %1 条").arg(deleted));
            onQueryHistory(); // 刷新表格
        }
    }
}

void MainWindow::onSaveConfig()
{
    // 从设置对话框获取值并保存到配置
    if (m_settingsDialog) {
        m_configMgr->setSerialBaudRate(m_settingsDialog->baudRate());
        m_configMgr->setServerAddress(m_settingsDialog->serverAddress());
        m_configMgr->setServerPort(static_cast<quint16>(m_settingsDialog->serverPort()));
        m_configMgr->setNetworkProtocol(m_settingsDialog->protocol());
        m_configMgr->setAutoUpload(m_settingsDialog->autoUpload());
        m_configMgr->setUploadInterval(m_settingsDialog->uploadInterval());
        m_configMgr->setCollectInterval(m_settingsDialog->collectInterval());
    }
    m_configMgr->saveConfig();
    showMessage("配置已保存");
    appendLog("[配置] 配置已保存到文件");
}

void MainWindow::onLoadConfig()
{
    QString file = QFileDialog::getOpenFileName(this, "加载配置文件", "", "INI Files (*.ini)");
    if (!file.isEmpty()) {
        m_configMgr->loadConfig(file);
        loadConfigToUI();
        showMessage("配置已加载: " + file);
        appendLog("[配置] 已加载配置: " + file);
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "关于 - 环境监测系统",
        "<h3>环境监测系统 v1.0</h3>"
        "<p>基于 Qt 的环境参数监测与管理系统</p>"
        "<p>功能模块：</p>"
        "<ul>"
        "<li>实时数据采集与展示（温度/湿度/光照）</li>"
        "<li>串口通信（支持自动识别与模拟模式）</li>"
        "<li>TCP/UDP 网络数据上传</li>"
        "<li>SQLite 历史数据存储与查询</li>"
        "</ul>"
        "<p>© 2026 EnvMonitor</p>");
}

// ==================== 错误与状态处理 ====================

void MainWindow::onSerialError(const QString &error)
{
    showMessage("串口错误: " + error, true);
    appendLog("[错误] 串口: " + error);
    LogManager::instance()->error("Serial", error);
}

void MainWindow::onNetworkError(const QString &error)
{
    showMessage("网络错误: " + error, true);
    appendLog("[错误] 网络: " + error);
    statusBar()->showMessage("⚠️ " + error, 5000);
    LogManager::instance()->error("Network", error);
}

void MainWindow::onDatabaseError(const QString &error)
{
    showMessage("数据库错误: " + error, true);
    appendLog("[错误] 数据库: " + error);
    LogManager::instance()->error("Database", error);
}

void MainWindow::onUploadSuccess()
{
    appendLog("[网络] 数据上传成功");
}

void MainWindow::onUploadFailed(const QString &error)
{
    appendLog("[错误] 上传失败: " + error);
    LogManager::instance()->warning("Network", "上传失败: " + error);
}

void MainWindow::showMessage(const QString &msg, bool isError)
{
    statusBar()->showMessage(msg, 3000);
    if (isError) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("提示");
        msgBox.setText(msg);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.addButton("确定", QMessageBox::AcceptRole);
        msgBox.exec();
    }
}

void MainWindow::appendLog(const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString colored;
    if (msg.contains("[错误]")) {
        colored = QString("<span style='color:#F87171;'>[%1] %2</span>").arg(timestamp, msg);
    } else if (msg.contains("[串口]") || msg.contains("[串口接收]")) {
        colored = QString("<span style='color:#4ADE80;'>[%1] %2</span>").arg(timestamp, msg);
    } else if (msg.contains("[网络]")) {
        colored = QString("<span style='color:#60A5FA;'>[%1] %2</span>").arg(timestamp, msg);
    } else if (msg.contains("[数据库]")) {
        colored = QString("<span style='color:#FBBF24;'>[%1] %2</span>").arg(timestamp, msg);
    } else if (msg.contains("[配置]")) {
        colored = QString("<span style='color:#A78BFA;'>[%1] %2</span>").arg(timestamp, msg);
    } else {
        colored = QString("<span style='color:#94A3B8;'>[%1] %2</span>").arg(timestamp, msg);
    }
    m_logView->append(colored);
}

void MainWindow::applyStyleSheet()
{
    setStyleSheet(R"(
        /* ========== 全局基础 ========== */
        QMainWindow {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #F8FAFC, stop:0.5 #F1F5F9, stop:1 #E2E8F0);
        }
        
        * {
            font-family: -apple-system, BlinkMacSystemFont, "Helvetica Neue", "PingFang SC", "Hiragino Sans GB", "Microsoft YaHei", sans-serif;
            font-size: 14px;
        }

        /* ========== 分组框 - 玻璃拟态卡片 ========== */
        QGroupBox {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(255,255,255,0.95), stop:1 rgba(248,250,252,0.9));
            border: 1px solid rgba(226,232,240,0.8);
            border-radius: 16px;
            margin-top: 20px;
            padding: 20px;
            padding-top: 32px;
            font-weight: 600;
            font-size: 14px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 20px;
            padding: 4px 12px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #3B82F6, stop:1 #60A5FA);
            color: white;
            border-radius: 8px;
            font-weight: 600;
            font-size: 14px;
        }

        /* ========== 选项卡 - 现代风格 ========== */
        QTabWidget::pane {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(255,255,255,0.98), stop:1 rgba(248,250,252,0.95));
            border: 1px solid rgba(226,232,240,0.8);
            border-radius: 16px;
            padding: 16px;
            margin-top: -1px;
        }
        QTabBar::tab {
            background: transparent;
            border: none;
            border-bottom: 3px solid transparent;
            padding: 12px 20px;
            margin-right: 4px;
            font-size: 13px;
            font-weight: 500;
            color: #64748B;
        }
        QTabBar::tab:selected {
            color: #3B82F6;
            border-bottom: 3px solid #3B82F6;
            font-weight: 600;
        }
        QTabBar::tab:hover:!selected {
            color: #475569;
            background: rgba(59,130,246,0.05);
            border-radius: 8px 8px 0 0;
        }

        /* ========== 按钮 - 现代扁平风格 ========== */
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #FFFFFF, stop:1 #F8FAFC);
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            padding: 10px 20px;
            font-size: 13px;
            font-weight: 500;
            color: #475569;
            min-height: 20px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F8FAFC, stop:1 #F1F5F9);
            border-color: #CBD5E1;
            color: #334155;
        }
        QPushButton:pressed {
            background: #F1F5F9;
            border-color: #94A3B8;
        }
        QPushButton:disabled {
            background: #F8FAFC;
            color: #94A3B8;
            border-color: #E2E8F0;
        }
        
        /* 主要按钮 */
        QPushButton#primaryBtn {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #3B82F6, stop:1 #2563EB);
            color: white;
            border: none;
            font-weight: 600;
            padding: 10px 24px;
        }
        QPushButton#primaryBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #60A5FA, stop:1 #3B82F6);
        }
        QPushButton#primaryBtn:pressed {
            background: #1D4ED8;
        }
        
        /* 危险按钮 */
        QPushButton#dangerBtn {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #EF4444, stop:1 #DC2626);
            color: white;
            border: none;
            font-weight: 600;
        }
        QPushButton#dangerBtn:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F87171, stop:1 #EF4444);
        }
        QPushButton#dangerBtn:pressed {
            background: #B91C1C;
        }

        /* ========== 输入控件 ========== */
        QComboBox, QSpinBox, QLineEdit, QDateTimeEdit {
            border: 2px solid #E2E8F0;
            border-radius: 10px;
            padding: 8px 12px;
            background: white;
            min-height: 20px;
            font-size: 13px;
            color: #334155;
            selection-background-color: #DBEAFE;
        }
        QComboBox:hover, QSpinBox:hover, QLineEdit:hover, QDateTimeEdit:hover {
            border-color: #CBD5E1;
        }
        QComboBox:focus, QSpinBox:focus, QLineEdit:focus, QDateTimeEdit:focus {
            border-color: #3B82F6;
            background: #FAFBFF;
        }
        QComboBox::drop-down {
            border: none;
            width: 30px;
            subcontrol-position: right center;
        }
        QComboBox::down-arrow {
            image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTIiIGhlaWdodD0iOCIgdmlld0JveD0iMCAwIDEyIDgiIGZpbGw9Im5vbmUiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PHBhdGggZD0iTTEgMS41TDYgNi41TDExIDEuNSIgc3Ryb2tlPSIjNjQ3NDhCIiBzdHJva2Utd2lkdGg9IjIiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIgc3Ryb2tlLWxpbmVqb2luPSJyb3VuZCIvPjwvc3ZnPg==);
            width: 12px;
            height: 8px;
            margin-right: 10px;
        }
        QComboBox QAbstractItemView {
            background: white;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            padding: 4px;
            selection-background-color: #DBEAFE;
            selection-color: #1E40AF;
        }

        /* ========== 表格 - 现代数据表格 ========== */
        QTableWidget {
            background: white;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
            gridline-color: #F1F5F9;
            font-size: 13px;
            selection-background-color: #DBEAFE;
            selection-color: #1E40AF;
        }
        QTableWidget::item {
            padding: 12px 8px;
            border-bottom: 1px solid #F1F5F9;
        }
        QTableWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #DBEAFE, stop:1 #BFDBFE);
            color: #1E40AF;
        }
        QTableWidget::item:hover {
            background: #F8FAFC;
        }
        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #F8FAFC, stop:1 #F1F5F9);
            border: none;
            border-bottom: 2px solid #3B82F6;
            padding: 12px 8px;
            font-weight: 600;
            font-size: 12px;
            color: #475569;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        QHeaderView::section:first {
            border-top-left-radius: 12px;
        }
        QHeaderView::section:last {
            border-top-right-radius: 12px;
        }

        /* ========== 进度条 - 渐变动效 ========== */
        QProgressBar {
            border: none;
            border-radius: 6px;
            text-align: center;
            background: #E2E8F0;
            height: 12px;
            font-size: 10px;
            font-weight: 600;
            color: #475569;
        }
        QProgressBar::chunk {
            border-radius: 6px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #3B82F6, stop:0.5 #60A5FA, stop:1 #3B82F6);
        }

        /* ========== 数值标签 ========== */
        QLabel#valueLabel {
            font-size: 20px;
            font-weight: 700;
            color: #1E293B;
            letter-spacing: -0.5px;
        }

        /* ========== 状态栏 ========== */
        QStatusBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #FFFFFF, stop:1 #F8FAFC);
            border-top: 1px solid #E2E8F0;
            font-size: 12px;
            padding: 4px 8px;
            color: #64748B;
        }
        QStatusBar::item {
            border: none;
        }

        /* ========== 工具栏 ========== */
        QToolBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #FFFFFF, stop:1 #F8FAFC);
            border: none;
            border-bottom: 1px solid #E2E8F0;
            spacing: 8px;
            padding: 8px 12px;
        }
        QToolBar QToolButton {
            background: transparent;
            border: none;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13px;
            color: #475569;
        }
        QToolBar QToolButton:hover {
            background: #F1F5F9;
            color: #3B82F6;
        }
        QToolBar QToolButton:pressed {
            background: #E2E8F0;
        }

        /* ========== 菜单栏 ========== */
        QMenuBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #FFFFFF, stop:1 #FAFBFC);
            border-bottom: 1px solid #E2E8F0;
            padding: 4px 8px;
            font-size: 13px;
        }
        QMenuBar::item {
            background: transparent;
            padding: 8px 16px;
            border-radius: 6px;
            color: #475569;
        }
        QMenuBar::item:selected {
            background: #F1F5F9;
            color: #3B82F6;
        }
        QMenu {
            background: white;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
            padding: 8px;
        }
        QMenu::item {
            padding: 10px 24px;
            border-radius: 6px;
            color: #475569;
        }
        QMenu::item:selected {
            background: #DBEAFE;
            color: #1E40AF;
        }
        QMenu::separator {
            height: 1px;
            background: #E2E8F0;
            margin: 6px 12px;
        }

        /* ========== 复选框 ========== */
        QCheckBox {
            font-size: 13px;
            spacing: 10px;
            color: #475569;
        }
        QCheckBox::indicator {
            width: 20px;
            height: 20px;
        }

        /* ========== 分割器 ========== */
        QSplitter::handle {
            background: transparent;
        }
        QSplitter::handle:horizontal {
            width: 8px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 transparent, stop:0.4 #E2E8F0, stop:0.6 #E2E8F0, stop:1 transparent);
        }
        QSplitter::handle:vertical {
            height: 8px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 transparent, stop:0.4 #E2E8F0, stop:0.6 #E2E8F0, stop:1 transparent);
        }
        QSplitter::handle:hover {
            background: #CBD5E1;
        }

        /* ========== 滚动条 ========== */
        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 4px 2px;
        }
        QScrollBar::handle:vertical {
            background: #CBD5E1;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #94A3B8;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            background: transparent;
            height: 10px;
            margin: 2px 4px;
        }
        QScrollBar::handle:horizontal {
            background: #CBD5E1;
            border-radius: 4px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #94A3B8;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }

        /* ========== 工具提示 ========== */
        QToolTip {
            background: #1E293B;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 12px;
        }
    )");
}
