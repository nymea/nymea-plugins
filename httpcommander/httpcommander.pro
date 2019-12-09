include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginhttpcommander)

SOURCES += \
    devicepluginhttpcommander.cpp \
    httpsimpleserver.cpp

HEADERS += \
    devicepluginhttpcommander.h \
    httpsimpleserver.h


