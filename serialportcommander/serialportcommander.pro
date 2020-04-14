include(../plugins.pri)

QT += serialport

TARGET = $$qtLibraryTarget(nymea_integrationpluginserialportcommander)

SOURCES += \
    integrationpluginserialportcommander.cpp \


HEADERS += \
    integrationpluginserialportcommander.h \
