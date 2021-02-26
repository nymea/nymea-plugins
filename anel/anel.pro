include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginanel)

SOURCES += \
    discovery.cpp \
    integrationpluginanel.cpp \

HEADERS += \
    discovery.h \
    integrationpluginanel.h \
