include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginsonos)

SOURCES += \
    integrationpluginsonos.cpp \
    sonos.cpp \

HEADERS += \
    integrationpluginsonos.h \
    sonos.h \
