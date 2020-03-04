include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginlgsmarttv)

QT+= network xml

SOURCES += \
    integrationpluginlgsmarttv.cpp \
    tvdevice.cpp \
    tveventhandler.cpp

HEADERS += \
    integrationpluginlgsmarttv.h \
    tvdevice.h \
    tveventhandler.h


