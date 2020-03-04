include(../plugins.pri)

QT += network

PKGCONFIG += nymea-mqtt

TARGET = $$qtLibraryTarget(nymea_integrationplugintuya)

SOURCES += \
    integrationplugintuya.cpp \

HEADERS += \
    integrationplugintuya.h \
