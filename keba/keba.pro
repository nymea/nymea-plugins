include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginkeba)

SOURCES += \
    integrationpluginkeba.cpp \
    kebadiscovery.cpp \
    kebaproductinfo.cpp \
    kecontact.cpp \
    kecontactdatalayer.cpp

HEADERS += \
    integrationpluginkeba.h \
    kebadiscovery.h \
    kebaproductinfo.h \
    kecontact.h \
    kecontactdatalayer.h
