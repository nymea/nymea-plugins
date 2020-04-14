include(../plugins.pri)

QT += \
    network    \
    websockets \

TARGET = $$qtLibraryTarget(nymea_integrationpluginbose)

SOURCES += \
    integrationpluginbose.cpp \
    soundtouch.cpp

HEADERS += \
    integrationpluginbose.h \
    soundtouch.h \
    soundtouchtypes.h
