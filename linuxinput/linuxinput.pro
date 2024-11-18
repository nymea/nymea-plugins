include(../plugins.pri)

PKGCONFIG += nymea-gpio

SOURCES += \
    integrationpluginlinuxinput.cpp \
    inputdevice.cpp

HEADERS += \
    integrationpluginlinuxinput.h \
    gpiodescriptor.h
