include(../plugins.pri)

PKGCONFIG += hidapi-hidraw libudev

SOURCES += \
    integrationpluginusbrelay.cpp \
    rawhiddevicewatcher.cpp \
    usbrelay.cpp

HEADERS += \
    integrationpluginusbrelay.h \
    rawhiddevicewatcher.h \
    usbrelay.h

