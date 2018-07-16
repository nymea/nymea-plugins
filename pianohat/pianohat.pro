include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginpianohat)

QT += multimedia

RESOURCES += sounds/sounds.qrc

SOURCES += \
    devicepluginpianohat.cpp \
    pianohat.cpp \
    touchsensor.cpp

HEADERS += \
    devicepluginpianohat.h \
    pianohat.h \
    touchsensor.h

