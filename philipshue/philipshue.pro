include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginphilipshue)

QT += network

SOURCES += \
    devicepluginphilipshue.cpp \
    #huebridgeconnection.cpp \
    #light.cpp \
    huebridge.cpp \
    huelight.cpp \
    huemotionsensor.cpp \
    hueremote.cpp \
    huedevice.cpp

HEADERS += \
    devicepluginphilipshue.h \
    #huebridgeconnection.h \
    #light.h \
    #lightinterface.h \
    huebridge.h \
    huelight.h \
    huemotionsensor.h \
    hueremote.h \
    huedevice.h



