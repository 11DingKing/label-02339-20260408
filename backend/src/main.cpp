#include <QApplication>
#include <QLocale>
#include <QDebug>
#include <QStyleFactory>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("EnvMonitor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("EnvMonitor");
    
    // 使用 Fusion 样式（跨平台一致性好，支持自定义）
    app.setStyle(QStyleFactory::create("Fusion"));

    // 设置全局字体
    QFont font = app.font();
    font.setFamily("Microsoft YaHei, PingFang SC, Helvetica Neue, sans-serif");
    font.setPixelSize(13);
    app.setFont(font);

    qInfo() << "========================================";
    qInfo() << "  环境监测系统 v1.0.0 启动";
    qInfo() << "========================================";

    MainWindow window;
    window.show();

    return app.exec();
}
