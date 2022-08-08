include(../plugins.pri)

QT += network

SOURCES += \
    integrationpluginsolarman.cpp \
    solarmandiscovery.cpp \
    solarmanmodbus.cpp \
    solarmanmodbusreply.cpp

HEADERS += \
    integrationpluginsolarman.h \
    solarmandiscovery.h \
    solarmanmodbus.h \
    solarmanmodbusreply.h

DISTFILES += \
    registermappings.json

RESOURCES += \
    registermappings.qrc
