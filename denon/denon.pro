include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationplugindenon)

SOURCES += \
    integrationplugindenon.cpp \
    heos.cpp \
    heosplayer.cpp \
    avrconnection.cpp

HEADERS += \
    integrationplugindenon.h \
    heos.h \
    heosplayer.h \
    avrconnection.h \
    heostypes.h
