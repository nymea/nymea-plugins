include(../plugins.pri)

QT += serialport

TARGET = $$qtLibraryTarget(nymea_devicepluginserialportcommander)

SOURCES += \
    devicepluginserialportcommander.cpp \
    serialportcommander.cpp


HEADERS += \
    devicepluginserialportcommander.h \
    serialportcommander.h
