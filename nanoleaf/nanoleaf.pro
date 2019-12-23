include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginnanoleaf)

QT += network

SOURCES += \
    devicepluginnanoleaf.cpp \
    nanoleaf.cpp \

HEADERS += \
    devicepluginnanoleaf.h \
    nanoleaf.h \



