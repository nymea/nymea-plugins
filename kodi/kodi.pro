include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginkodi)

SOURCES += \
    integrationpluginkodi.cpp \
    kodiconnection.cpp \
    kodijsonhandler.cpp \
    kodi.cpp \
    kodireply.cpp

HEADERS += \
    integrationpluginkodi.h \
    kodiconnection.h \
    kodijsonhandler.h \
    kodi.h \
    kodireply.h

