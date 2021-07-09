include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginlgsmarttv)

QT+= network xml websockets

SOURCES += \
    integrationpluginlgsmarttv.cpp \
    tvdevice.cpp \
    tveventhandler.cpp \
    webosconnection.cpp

HEADERS += \
    integrationpluginlgsmarttv.h \
    tvdevice.h \
    tveventhandler.h \
    webosconnection.h


