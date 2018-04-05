include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_devicepluginelgato)

SOURCES += \
    devicepluginelgato.cpp \
    aveabulb.cpp \

HEADERS += \
    devicepluginelgato.h \
    aveabulb.h \



