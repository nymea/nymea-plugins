include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_devicepluginsenic)

SOURCES += \
    devicepluginsenic.cpp \
    nuimo.cpp

HEADERS += \
    devicepluginsenic.h \
    nuimo.h


