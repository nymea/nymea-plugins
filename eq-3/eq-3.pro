include(../plugins.pri)

QT += network bluetooth

TARGET = $$qtLibraryTarget(nymea_integrationplugineq-3)

SOURCES += \
    integrationplugineq-3.cpp    \
    maxcubediscovery.cpp    \
    maxcube.cpp             \
    maxdevice.cpp           \
    room.cpp \
    wallthermostat.cpp \
    radiatorthermostat.cpp \
    eqivabluetooth.cpp

HEADERS += \
    integrationplugineq-3.h      \
    maxcubediscovery.h      \
    maxcube.h               \
    maxdevice.h             \
    room.h \
    wallthermostat.h \
    radiatorthermostat.h \
    eqivabluetooth.h

