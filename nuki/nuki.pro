include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginnuki)

QT += bluetooth dbus

# apt install libsodium-dev
LIBS += -lsodium

HEADERS += \
    integrationpluginnuki.h \
    nuki.h \
    bluez/blueztypes.h \
    bluez/bluetoothmanager.h \
    bluez/bluetoothadapter.h \
    bluez/bluetoothdevice.h \
    bluez/bluetoothgattservice.h \
    bluez/bluetoothgattcharacteristic.h \
    bluez/bluetoothgattdescriptor.h \
    nukiutils.h \
    nukiauthenticator.h \
    nukicontroller.h

SOURCES += \
    integrationpluginnuki.cpp \
    nuki.cpp \
    bluez/blueztypes.cpp \
    bluez/bluetoothmanager.cpp \
    bluez/bluetoothadapter.cpp \
    bluez/bluetoothdevice.cpp \
    bluez/bluetoothgattservice.cpp \
    bluez/bluetoothgattcharacteristic.cpp \
    bluez/bluetoothgattdescriptor.cpp \
    nukiutils.cpp \
    nukiauthenticator.cpp \
    nukicontroller.cpp
