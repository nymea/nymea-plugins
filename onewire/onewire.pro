include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginonewire)

LIBS +=      \
        -low \
        -lowcapi \

SOURCES += \
    devicepluginonewire.cpp \
    onewire.cpp \

HEADERS += \
    devicepluginonewire.h \
    onewire.h \


