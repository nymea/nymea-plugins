include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationplugindatetime)

SOURCES += \
    integrationplugindatetime.cpp \
    alarm.cpp \
    countdown.cpp

HEADERS += \
    integrationplugindatetime.h \
    alarm.h \
    countdown.h

