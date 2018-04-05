include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_deviceplugindatetime)

SOURCES += \
    deviceplugindatetime.cpp \
    alarm.cpp \
    countdown.cpp

HEADERS += \
    deviceplugindatetime.h \
    alarm.h \
    countdown.h

