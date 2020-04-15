include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginhomeconnect)

SOURCES += \
    integrationpluginhomeconnect.cpp \
    homeconnect.cpp \

HEADERS += \
    integrationpluginhomeconnect.h \
    homeconnect.h \
