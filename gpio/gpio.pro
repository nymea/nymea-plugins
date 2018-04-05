include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_deviceplugingpio)

SOURCES += \
    deviceplugingpio.cpp \
    gpiodescriptor.cpp

HEADERS += \
    deviceplugingpio.h \
    gpiodescriptor.h


