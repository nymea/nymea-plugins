include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginlgsmarttv)

QT+= network xml websockets

SOURCES += \
    devicepluginlgsmarttv.cpp \
    tvdevice.cpp \
    tveventhandler.cpp \
    webosconnection.cpp

HEADERS += \
    devicepluginlgsmarttv.h \
    tvdevice.h \
    tveventhandler.h \
    webosconnection.h


