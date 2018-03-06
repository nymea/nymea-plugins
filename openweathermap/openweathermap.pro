include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginopenweathermap)

QT+= network

SOURCES += \
    devicepluginopenweathermap.cpp \

HEADERS += \
    devicepluginopenweathermap.h \


