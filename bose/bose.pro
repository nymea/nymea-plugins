include(../plugins.pri)

QT += \
    network    \
    websockets \

TARGET = $$qtLibraryTarget(nymea_devicepluginbose)

SOURCES += \
    devicepluginbose.cpp \
    soundtouch.cpp

HEADERS += \
    devicepluginbose.h \
    soundtouch.h \
    soundtouchtypes.h
