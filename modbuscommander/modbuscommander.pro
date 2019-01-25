include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginmodbuscommander)

QT += serialport
QT += network

LIBS += -lmodbus

SOURCES += \
    devicepluginmodbuscommander.cpp \  
    modbustcpmaster.cpp \
    modbusrtumaster.cpp

HEADERS += \
    devicepluginmodbuscommander.h \
    modbustcpmaster.h \
    modbusrtumaster.h
