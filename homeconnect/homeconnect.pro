include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginhomeconnect)

SOURCES += \
    devicepluginhomeconnect.cpp \
    homeconnect.cpp \

HEADERS += \
    devicepluginhomeconnect.h \
    homeconnect.h \
