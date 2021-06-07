include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginkeba)

SOURCES += \
    integrationpluginkeba.cpp \
    kecontact.cpp \
    kecontactdatalayer.cpp

HEADERS += \
    integrationpluginkeba.h \
    kecontact.h \
    kecontactdatalayer.h
