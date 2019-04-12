include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_deviceplugintexasinstruments)

HEADERS += \
    deviceplugintexasinstruments.h \
    sensortag.h \
    sensordataprocessor.h \
    sensorfilter.h

SOURCES += \
    deviceplugintexasinstruments.cpp \
    sensortag.cpp \
    sensordataprocessor.cpp \
    sensorfilter.cpp


