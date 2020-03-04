include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_integrationpluginflowercare)

SOURCES += \
    integrationpluginflowercare.cpp \
    flowercare.cpp

HEADERS += \
    integrationpluginflowercare.h \
    flowercare.h
