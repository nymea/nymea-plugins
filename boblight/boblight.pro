include(../plugins.pri)

QT += dbus bluetooth concurrent

CONFIG += c++11

TARGET = $$qtLibraryTarget(nymea_integrationpluginboblight)
LIBS += -lboblight

SOURCES += \
    integrationpluginboblight.cpp \
    bobclient.cpp \
    bobchannel.cpp

HEADERS += \
    integrationpluginboblight.h \
    bobclient.h \
    bobchannel.h


