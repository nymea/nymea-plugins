include(../plugins.pri)

QT += network serialport

SOURCES += \
    integrationpluginowlet.cpp \
    owletclient.cpp \
    owletserialtransport.cpp \
    owlettcptransport.cpp \
    owlettransport.cpp

HEADERS += \
    integrationpluginowlet.h \
    owletclient.h \
    owletserialtransport.h \
    owlettcptransport.h \
    owlettransport.h


