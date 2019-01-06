include(/usr/include/nymea/plugin.pri)

QT += dbus bluetooth concurrent

CONFIG += c++11

TARGET = $$qtLibraryTarget(nymea_devicepluginboblight)
LIBS += -lboblight

SOURCES += \
    devicepluginboblight.cpp \
    bobclient.cpp \
    bobchannel.cpp

HEADERS += \
    devicepluginboblight.h \
    bobclient.h \
    bobchannel.h


