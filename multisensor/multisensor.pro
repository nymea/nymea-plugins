include(../plugins.pri)

TARGET = $$qtLibraryTarget(guh_devicepluginmultisensor)

SOURCES += \
    devicepluginmultisensor.cpp \
    #sensortag-old.cpp \
    sensortag.cpp

HEADERS += \
    devicepluginmultisensor.h \
    #sensortag-old.h \
    sensortag.h
