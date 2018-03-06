include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginmultisensor)

SOURCES += \
    devicepluginmultisensor.cpp \
    #sensortag-old.cpp \
    sensortag.cpp

HEADERS += \
    devicepluginmultisensor.h \
    #sensortag-old.h \
    sensortag.h
