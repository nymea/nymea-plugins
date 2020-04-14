include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginhttpcommander)

SOURCES += \
    integrationpluginhttpcommander.cpp \
    httpsimpleserver.cpp

HEADERS += \
    integrationpluginhttpcommander.h \
    httpsimpleserver.h


