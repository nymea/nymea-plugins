include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginunipi)

QT += \
    network \
    serialport \
    concurrent \
    core \
    serialbus \

LIBS += -lmodbus

SOURCES += \
    devicepluginunipi.cpp \
    dimmerswitch.cpp \
    modbustcpmaster.cpp \
    neuron.cpp \
    neuronextension.cpp

HEADERS += \
    devicepluginunipi.h \
    dimmerswitch.h \
    modbustcpmaster.h \
    neuron.h \
    neuronextension.h

MAP_FILES.files = modbus_maps/
MAP_FILES.path = /usr/share/nymea/modbus/
INSTALLS += MAP_FiLES

