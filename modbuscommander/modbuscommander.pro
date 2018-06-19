include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginmodbuscommander)

QT += serialport
QT += network

LIBS += -lmodbus

SOURCES += \
    devicepluginmodbuscommander.cpp \ 
    modbustcpclient.cpp

HEADERS += \
    devicepluginmodbuscommander.h \
    modbustcpclient.h
