include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginkodi)

SOURCES += \
    devicepluginkodi.cpp \
    kodiconnection.cpp \
    kodijsonhandler.cpp \
    kodi.cpp \
    kodireply.cpp

HEADERS += \
    devicepluginkodi.h \
    kodiconnection.h \
    kodijsonhandler.h \
    kodi.h \
    kodireply.h

