include(../plugins.pri)

#QT += gui

TARGET = $$qtLibraryTarget(nymea_devicepluginkeyevent)

SOURCES += \
    devicepluginkeyevent.cpp \
    emitkeyevent.cpp \
    receivekeyevent.cpp

HEADERS += \
    devicepluginkeyevent.h \
    emitkeyevent.h \
    keymap.h \
    receivekeyevent.h

