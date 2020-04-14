include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_integrationpluginelgato)

SOURCES += \
    integrationpluginelgato.cpp \
    aveabulb.cpp \

HEADERS += \
    integrationpluginelgato.h \
    aveabulb.h \



