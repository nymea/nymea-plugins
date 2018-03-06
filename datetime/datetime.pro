include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_deviceplugindatetime)

SOURCES += \
    deviceplugindatetime.cpp \
    alarm.cpp \
    countdown.cpp

HEADERS += \
    deviceplugindatetime.h \
    alarm.h \
    countdown.h

