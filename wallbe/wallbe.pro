include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginwallbe)

QT += network

LIBS += -lmodbus

SOURCES += \
    devicepluginwallbe.cpp \
    wallbe.cpp \
    host.cpp \
    discover.cpp

HEADERS += \
    devicepluginwallbe.h \
    wallbe.h \
    host.h \
    discover.h
