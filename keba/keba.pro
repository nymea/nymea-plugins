include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginkeba)

SOURCES += \
    devicepluginkeba.cpp \
    discovery.cpp \
    host.cpp \

HEADERS += \
    devicepluginkeba.h \
    discovery.h \
    host.h \
