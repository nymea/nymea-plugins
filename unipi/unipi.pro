include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginunipi)

LIBS += -li2c

QT += \
    network \
    serialport \
    serialbus \

SOURCES += \
    devicepluginunipi.cpp \
    neuron.cpp \
    neuronextension.cpp \
    mcp23008.cpp \
    i2cport.cpp \
    unipi.cpp \
    mcp3422.cpp

HEADERS += \
    devicepluginunipi.h \
    neuron.h \
    neuronextension.h \
    mcp23008.h \
    i2cport.h \
    unipi.h \
    i2cport_p.h \
    mcp3422.h

MAP_FILES.files = files(modbus_maps/*)
MAP_FILES.path = [QT_INSTALL_PREFIX]/share/nymea/modbus/
INSTALLS += MAP_FILES

