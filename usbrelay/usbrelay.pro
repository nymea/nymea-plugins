include(../plugins.pri)

PKGCONFIG += hidapi-hidraw libudev

SOURCES += \
    devicepluginusbrelay.cpp \
    rawhiddevicewatcher.cpp \
    usbrelay.cpp

HEADERS += \
    devicepluginusbrelay.h \
    rawhiddevicewatcher.h \
    usbrelay.h

