include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginpianohat)

SOURCES += \
    devicepluginpianohat.cpp \
    pianohat.cpp \
    touchsensor.cpp

HEADERS += \
    devicepluginpianohat.h \
    pianohat.h \
    touchsensor.h

