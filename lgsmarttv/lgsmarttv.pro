include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginlgsmarttv)

QT+= network xml

SOURCES += \
    devicepluginlgsmarttv.cpp \
    tvdevice.cpp \
    tveventhandler.cpp

HEADERS += \
    devicepluginlgsmarttv.h \
    tvdevice.h \
    tveventhandler.h


