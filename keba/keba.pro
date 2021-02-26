include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginkeba)

SOURCES += \
    integrationpluginkeba.cpp \
    kecontact.cpp \
    discovery.cpp \
    host.cpp \
    kecontactdatalayer.cpp

HEADERS += \
    integrationpluginkeba.h \
    kecontact.h \
    discovery.h \
    host.h \
    kecontactdatalayer.h
