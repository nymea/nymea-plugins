TRANSLATIONS = translations/en_US.ts \
               translations/de_DE.ts

# Note: include after the TRANSLATIONS definition
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
