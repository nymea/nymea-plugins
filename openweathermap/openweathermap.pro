include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginopenweathermap)

QT+= network

SOURCES += \
    integrationpluginopenweathermap.cpp \

HEADERS += \
    integrationpluginopenweathermap.h \


