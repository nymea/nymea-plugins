include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginkeba)

SOURCES += \
    integrationpluginkeba.cpp \
    kebadiscovery.cpp \
    kecontact.cpp \
    kecontactdatalayer.cpp

HEADERS += \
    integrationpluginkeba.h \
    kebadiscovery.h \
    kecontact.h \
    kecontactdatalayer.h
