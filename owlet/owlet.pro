include(../plugins.pri)

QT += network serialport

SOURCES += \
    arduinoflasher.cpp \
    integrationpluginowlet.cpp \
    owletclient.cpp \
    owletserialclient.cpp \
    owletserialtransport.cpp \
    owlettcptransport.cpp \
    owlettransport.cpp

HEADERS += \
    arduinoflasher.h \
    integrationpluginowlet.h \
    owletclient.h \
    owletserialclient.h \
    owletserialtransport.h \
    owlettcptransport.h \
    owlettransport.h


