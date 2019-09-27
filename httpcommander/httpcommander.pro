include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginhttpcommander)

SOURCES += \
    devicepluginhttpcommander.cpp \
    httprequest.cpp

HEADERS += \
    devicepluginhttpcommander.h \
    httprequest.h


