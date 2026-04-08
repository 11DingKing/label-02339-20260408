QT       += core gui widgets serialport network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/managers/serialmanager.cpp \
    src/managers/networkmanager.cpp \
    src/managers/databasemanager.cpp \
    src/managers/configmanager.cpp \
    src/core/dataprocessor.cpp \
    src/models/sensordata.cpp \
    src/widgets/gaugewidget.cpp

HEADERS += \
    src/mainwindow.h \
    src/managers/serialmanager.h \
    src/managers/networkmanager.h \
    src/managers/databasemanager.h \
    src/managers/configmanager.h \
    src/core/dataprocessor.h \
    src/models/sensordata.h \
    src/widgets/gaugewidget.h

RESOURCES += \
    resources/resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
