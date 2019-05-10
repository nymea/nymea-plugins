include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_deviceplugindenon)

SOURCES += \
    deviceplugindenon.cpp \
    denonconnection.cpp \
    heos.cpp \
    heosplayer.cpp \

HEADERS += \
    deviceplugindenon.h \
    denonconnection.h \
    heos.h \
    heosplayer.h \
