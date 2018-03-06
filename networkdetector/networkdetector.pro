include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginnetworkdetector)

SOURCES += \
    devicepluginnetworkdetector.cpp \
    host.cpp \
    discovery.cpp \
    devicemonitor.cpp

HEADERS += \
    devicepluginnetworkdetector.h \
    host.h \
    discovery.h \
    devicemonitor.h


