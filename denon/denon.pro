include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_deviceplugindenon)

SOURCES += \
    deviceplugindenon.cpp \
    denonconnection.cpp

HEADERS += \
    deviceplugindenon.h \
    denonconnection.h
