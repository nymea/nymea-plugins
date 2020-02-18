include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginkeba)

SOURCES += \
    devicepluginkeba.cpp \
    kecontact.cpp \
    discovery.cpp \
    host.cpp \

HEADERS += \
    devicepluginkeba.h \
    kecontact.h \
    discovery.h \
    host.h \
