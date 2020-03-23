include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginlifx)

QT += network

SOURCES += \
    integrationpluginlifx.cpp \
    lifx.cpp \

HEADERS += \
    integrationpluginlifx.h \
    lifx.h \

