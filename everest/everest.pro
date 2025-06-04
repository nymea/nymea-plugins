include(../plugins.pri)

QT += network websockets
PKGCONFIG += nymea-mqtt

SOURCES += \
    jsonrpc/everestjsonrpcclient.cpp \
    jsonrpc/everestjsonrpcinterface.cpp \
    jsonrpc/everestjsonrpcreply.cpp \
    mqtt/everestmqtt.cpp \
    mqtt/everestmqttclient.cpp \
    mqtt/everestmqttdiscovery.cpp \
    integrationplugineverest.cpp

HEADERS += \
    jsonrpc/everestjsonrpcclient.h \
    jsonrpc/everestjsonrpcinterface.h \
    jsonrpc/everestjsonrpcreply.h \
    mqtt/everestmqtt.h \
    mqtt/everestmqttclient.h \
    mqtt/everestmqttdiscovery.h \
    integrationplugineverest.h
