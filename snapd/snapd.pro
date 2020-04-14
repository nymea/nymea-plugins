include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginsnapd)

QT += network

SOURCES += \
    integrationpluginsnapd.cpp \
    snapdcontrol.cpp \
    snapdconnection.cpp \
    snapdreply.cpp

HEADERS += \
    integrationpluginsnapd.h \
    snapdcontrol.h \
    snapdconnection.h \
    snapdreply.h
