include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginunipi)

QT += \
    network \
    serialport \
    core \
    serialbus \

SOURCES += \
    devicepluginunipi.cpp \
    neuron.cpp \
    neuronextension.cpp \
    mcp23008.cpp \
    i2cport.cpp \
    unipi.cpp \
    gpiodescriptor.cpp

HEADERS += \
    devicepluginunipi.h \
    neuron.h \
    neuronextension.h \
    mcp23008.h \
    i2cport.h \
    unipi.h \
    gpiodescriptor.h \
    i2cport_p.h

MAP_FILES.files = modbus_maps/
MAP_FILES.path = /usr/share/nymea/modbus/
INSTALLS += MAP_FiLES

