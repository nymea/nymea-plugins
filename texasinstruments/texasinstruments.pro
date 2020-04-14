include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_integrationplugintexasinstruments)

HEADERS += \
    integrationplugintexasinstruments.h \
    sensortag.h \
    sensordataprocessor.h \
    sensorfilter.h

SOURCES += \
    integrationplugintexasinstruments.cpp \
    sensortag.cpp \
    sensordataprocessor.cpp \
    sensorfilter.cpp


