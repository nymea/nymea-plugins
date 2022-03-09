include(../plugins.pri)

QT *= serialport

PKGCONFIG += libudev

HEADERS += \
    integrationpluginusbrly82.h \
    serialportmonitor.h \
    usbrly82.h

SOURCES += \
    integrationpluginusbrly82.cpp \
    serialportmonitor.cpp \
    usbrly82.cpp

