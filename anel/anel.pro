include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginanel)

SOURCES += \
    devicepluginanel.cpp \
    anelpanel.cpp

HEADERS += \
    devicepluginanel.h \
    anelpanel.h
