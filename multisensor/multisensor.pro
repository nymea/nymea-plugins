include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_devicepluginmultisensor)

SOURCES += \
    devicepluginmultisensor.cpp \
    #sensortag-old.cpp \
    sensortag.cpp \
    sensorfilter.cpp \
    sensordataprocessor.cpp

HEADERS += \
    devicepluginmultisensor.h \
    #sensortag-old.h \
    sensortag.h \
    sensorfilter.h \
    sensordataprocessor.h
