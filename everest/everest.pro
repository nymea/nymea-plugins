include(../plugins.pri)

QT += network websockets
PKGCONFIG += nymea-mqtt

SOURCES += \
    mqtt/everestmqtt.cpp \
    mqtt/everestmqttclient.cpp \
    mqtt/everestmqttdiscovery.cpp \
    integrationplugineverest.cpp \
    jsonrpc/everestjsonrpcclient.cpp

HEADERS += \
    mqtt/everestmqtt.h \
    mqtt/everestmqttclient.h \
    mqtt/everestmqttdiscovery.h \
    integrationplugineverest.h \
    jsonrpc/everestjsonrpcclient.h
