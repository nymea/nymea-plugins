include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_devicepluginflowercare)

SOURCES += \
    devicepluginflowercare.cpp \
    flowercare.cpp

HEADERS += \
    devicepluginflowercare.h \
    flowercare.h
