include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginhoneywellscanner)

QT += serialport

SOURCES += \
    devicepluginhoneywellscanner.cpp \
    honeywellscanner.cpp

HEADERS += \
    devicepluginhoneywellscanner.h \
    honeywellscanner.h

