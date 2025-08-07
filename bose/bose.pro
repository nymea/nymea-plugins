include(../plugins.pri)

QT *= network websockets

SOURCES += \
    integrationpluginbose.cpp \
    soundtouch.cpp

HEADERS += \
    integrationpluginbose.h \
    soundtouch.h \
    soundtouchtypes.h
