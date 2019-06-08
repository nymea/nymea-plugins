include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginunipi)

QT += \
    network \
    serialport \
    core \
    serialbus \

SOURCES += \
    devicepluginunipi.cpp \
    dimmerswitch.cpp \
    neuron.cpp \
    neuronextension.cpp

HEADERS += \
    devicepluginunipi.h \
    dimmerswitch.h \
    neuron.h \
    neuronextension.h

MAP_FILES.files = modbus_maps/
MAP_FILES.path = /usr/share/nymea/modbus/
INSTALLS += MAP_FiLES

