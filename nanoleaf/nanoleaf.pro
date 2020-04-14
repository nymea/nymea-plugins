include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginnanoleaf)

QT += network

SOURCES += \
    integrationpluginnanoleaf.cpp \
    nanoleaf.cpp \

HEADERS += \
    integrationpluginnanoleaf.h \
    nanoleaf.h \



