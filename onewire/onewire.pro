include(../plugins.pri)

LIBS += -low -lowcapi

SOURCES += \
    integrationpluginonewire.cpp \
    owfs.cpp \
    w1.cpp \

HEADERS += \
    integrationpluginonewire.h \
    owfs.h \
    w1.h \

