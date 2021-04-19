include(../plugins.pri)

QT += network

PKGCONFIG += nymea-mqtt

SOURCES += \
    integrationpluginshelly.cpp \

HEADERS += \
    integrationpluginshelly.h \
