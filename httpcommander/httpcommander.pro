include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginhttpcommander)

SOURCES += \
    devicepluginhttpcommander.cpp \
    httprequest.cpp \
    httpsimpleserver.cpp

HEADERS += \
    devicepluginhttpcommander.h \
    httprequest.h \
    httpsimpleserver.h


