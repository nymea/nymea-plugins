include(../plugins.pri)

QT += network

PKGCONFIG += nymea-mqtt

TARGET = $$qtLibraryTarget(nymea_deviceplugintuya)

SOURCES += \
    deviceplugintuya.cpp \

HEADERS += \
    deviceplugintuya.h \
