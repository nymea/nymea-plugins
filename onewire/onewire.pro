include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginonewire)

LIBS +=      \
        -low \
        -lowcapi \

SOURCES += \
    integrationpluginonewire.cpp \
    onewire.cpp \

HEADERS += \
    integrationpluginonewire.h \
    onewire.h \


