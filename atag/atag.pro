include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginatag)

SOURCES += \
    integrationpluginatag.cpp \
    atag.cpp \

HEADERS += \
    integrationpluginatag.h \
    atag.h \
