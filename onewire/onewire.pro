include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginonewire)

PKGCONFIG += owcapi

SOURCES += \
    integrationpluginonewire.cpp \
    owfs.cpp \
    w1.cpp \

HEADERS += \
    integrationpluginonewire.h \
    owfs.h \
    w1.h \

