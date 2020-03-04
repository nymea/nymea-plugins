include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginawattar)

SOURCES += \
    integrationpluginawattar.cpp \
    heatpump.cpp

HEADERS += \
    integrationpluginawattar.h \
    heatpump.h


