include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginusbrelay)

PKGCONFIG += hidapi-hidraw udev

SOURCES += \
    devicepluginusbrelay.cpp \
    rawhiddevicewatcher.cpp \
    usbrelay.cpp

HEADERS += \
    devicepluginusbrelay.h \
    rawhiddevicewatcher.h \
    usbrelay.h

