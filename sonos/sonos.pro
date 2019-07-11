include(../plugins.pri)

QT += network

LIBS += -lnoson

TARGET = $$qtLibraryTarget(nymea_devicepluginsonos)

SOURCES += \
    devicepluginsonos.cpp \

HEADERS += \
    devicepluginsonos.h \



