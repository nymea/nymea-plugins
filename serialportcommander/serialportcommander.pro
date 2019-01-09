include(../plugins.pri)

QT += serialport

TARGET = $$qtLibraryTarget(nymea_devicepluginserialportcommander)

SOURCES += \
    devicepluginserialportcommander.cpp \


HEADERS += \
    devicepluginserialportcommander.h \
