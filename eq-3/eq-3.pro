include(../plugins.pri)

QT += network bluetooth

TARGET = $$qtLibraryTarget(nymea_deviceplugineq-3)

SOURCES += \
    deviceplugineq-3.cpp    \
    maxcubediscovery.cpp    \
    maxcube.cpp             \
    maxdevice.cpp           \
    room.cpp \
    wallthermostat.cpp \
    radiatorthermostat.cpp \
    eqivabluetooth.cpp

HEADERS += \
    deviceplugineq-3.h      \
    maxcubediscovery.h      \
    maxcube.h               \
    maxdevice.h             \
    room.h \
    wallthermostat.h \
    radiatorthermostat.h \
    eqivabluetooth.h

