include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginbluos)

SOURCES += \
    integrationpluginbluos.cpp \
    bluos.cpp \

HEADERS += \
    integrationpluginbluos.h \
    bluos.h \
