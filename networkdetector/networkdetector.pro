include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginnetworkdetector)

SOURCES += \
    integrationpluginnetworkdetector.cpp \
    host.cpp \
    discovery.cpp \
    devicemonitor.cpp \
    broadcastping.cpp

HEADERS += \
    integrationpluginnetworkdetector.h \
    host.h \
    discovery.h \
    devicemonitor.h \
    broadcastping.h


