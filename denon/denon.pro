include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_deviceplugindenon)

SOURCES += \
    deviceplugindenon.cpp \
    heos.cpp \
    heosplayer.cpp \
    avrconnection.cpp

HEADERS += \
    deviceplugindenon.h \
    heos.h \
    heosplayer.h \
    avrconnection.h \
    heostypes.h
