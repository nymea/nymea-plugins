include(../plugins.pri)

QT += network serialport

SOURCES += \
    integrationpluginowlet.cpp \
    owletclient.cpp \
    owletserialclient.cpp \
    owletserialtransport.cpp \
    owlettcptransport.cpp \
    owlettransport.cpp

HEADERS += \
    integrationpluginowlet.h \
    owletclient.h \
    owletserialclient.h \
    owletserialtransport.h \
    owlettcptransport.h \
    owlettransport.h


