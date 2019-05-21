include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginunipi)

SOURCES += \
    devicepluginunipi.cpp \
    dimmerswitch.cpp \
    modbustcpmaster.cpp

HEADERS += \
    devicepluginunipi.h \
    dimmerswitch.h \
    modbustcpmaster.h


