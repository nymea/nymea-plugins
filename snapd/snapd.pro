include(../plugins.pri)

TARGET = $$qtLibraryTarget(guh_devicepluginsnapd)

QT += network

SOURCES += \
    devicepluginsnapd.cpp \
    snapdcontrol.cpp \
    snapdconnection.cpp \
    snapdreply.cpp

HEADERS += \
    devicepluginsnapd.h \
    snapdcontrol.h \
    snapdconnection.h \
    snapdreply.h
