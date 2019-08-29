include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginsonos)

SOURCES += \
    devicepluginsonos.cpp \
    sonos.cpp \

HEADERS += \
    devicepluginsonos.h \
    sonos.h \


