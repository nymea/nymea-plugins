include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginnetatmo)

SOURCES += \
    devicepluginnetatmo.cpp \
    netatmobasestation.cpp \
    netatmooutdoormodule.cpp

HEADERS += \
    devicepluginnetatmo.h \
    netatmobasestation.h \
    netatmooutdoormodule.h


