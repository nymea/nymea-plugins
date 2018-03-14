include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginnetatmo)

SOURCES += \
    devicepluginnetatmo.cpp \
    netatmobasestation.cpp \
    netatmooutdoormodule.cpp

HEADERS += \
    devicepluginnetatmo.h \
    netatmobasestation.h \
    netatmooutdoormodule.h


