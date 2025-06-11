include(../plugins.pri)

QT += network websockets
PKGCONFIG += nymea-mqtt

SOURCES += \
    jsonrpc/everestconnection.cpp \
    jsonrpc/everestjsonrpcclient.cpp \
    jsonrpc/everestjsonrpcdiscovery.cpp \
    jsonrpc/everestjsonrpcinterface.cpp \
    jsonrpc/everestjsonrpcreply.cpp \
    mqtt/everestmqtt.cpp \
    mqtt/everestmqttclient.cpp \
    mqtt/everestmqttdiscovery.cpp \
    integrationplugineverest.cpp

HEADERS += \
    jsonrpc/everestconnection.h \
    jsonrpc/everestjsonrpcclient.h \
    jsonrpc/everestjsonrpcdiscovery.h \
    jsonrpc/everestjsonrpcinterface.h \
    jsonrpc/everestjsonrpcreply.h \
    mqtt/everestmqtt.h \
    mqtt/everestmqttclient.h \
    mqtt/everestmqttdiscovery.h \
    integrationplugineverest.h
