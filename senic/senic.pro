include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_integrationpluginsenic)

SOURCES += \
    integrationpluginsenic.cpp \
    nuimo.cpp

HEADERS += \
    integrationpluginsenic.h \
    nuimo.h


