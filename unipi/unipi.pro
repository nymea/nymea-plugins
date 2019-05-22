include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginunipi)

QT += \
    network \
    serialport \

SOURCES += \
    devicepluginunipi.cpp \
    dimmerswitch.cpp \
    modbustcpmaster.cpp \
    modbusrtumaster.cpp \

HEADERS += \
    devicepluginunipi.h \
    dimmerswitch.h \
    modbustcpmaster.h \
    modbusrtumaster.h \


