include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginphilipshue)

QT += network

SOURCES += \
    integrationpluginphilipshue.cpp \
    #huebridgeconnection.cpp \
    #light.cpp \
    huebridge.cpp \
    huelight.cpp \
    huemotionsensor.cpp \
    hueremote.cpp \
    huedevice.cpp

HEADERS += \
    integrationpluginphilipshue.h \
    #huebridgeconnection.h \
    #light.h \
    #lightinterface.h \
    huebridge.h \
    huelight.h \
    huemotionsensor.h \
    hueremote.h \
    huedevice.h



