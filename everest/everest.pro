include(../plugins.pri)

QT += network
PKGCONFIG += nymea-mqtt

SOURCES += \
    everest.cpp \
    everestclient.cpp \
    everestdiscovery.cpp \
    integrationplugineverest.cpp

HEADERS += \
    everest.h \
    everestclient.h \
    everestdiscovery.h \
    integrationplugineverest.h

