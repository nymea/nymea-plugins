include(../plugins.pri)

QT += network

SOURCES += \
    integrationpluginsma.cpp \
    speedwirediscovery.cpp \
    speedwireinterface.cpp \
    speedwireinverter.cpp \
    speedwireinverterreply.cpp \
    speedwireinverterrequest.cpp \
    speedwiremeter.cpp \
    sunnywebbox.cpp

HEADERS += \
    integrationpluginsma.h \
    speedwire.h \
    speedwirediscovery.h \
    speedwireinterface.h \
    speedwireinverter.h \
    speedwireinverterreply.h \
    speedwireinverterrequest.h \
    speedwiremeter.h \
    sunnywebbox.h
