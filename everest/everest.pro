include(../plugins.pri)

QT += network websockets dbus
PKGCONFIG += nymea-mqtt

SOURCES += \
    rfid/rfidproviderclient.cpp \
    jsonrpc/everestconnection.cpp \
    jsonrpc/everestevse.cpp \
    jsonrpc/everestjsonrpcclient.cpp \
    jsonrpc/everestjsonrpcdiscovery.cpp \
    jsonrpc/everestjsonrpcinterface.cpp \
    jsonrpc/everestjsonrpcreply.cpp \
    mqtt/everestmqtt.cpp \
    mqtt/everestmqttclient.cpp \
    mqtt/everestmqttdiscovery.cpp \
    integrationplugineverest.cpp

HEADERS += \
    rfid/rfidproviderclient.h \
    rfid/everestrfidutils.h \
    jsonrpc/everestconnection.h \
    jsonrpc/everestevse.h \
    jsonrpc/everestjsonrpcclient.h \
    jsonrpc/everestjsonrpcdiscovery.h \
    jsonrpc/everestjsonrpcinterface.h \
    jsonrpc/everestjsonrpcreply.h \
    mqtt/everestmqtt.h \
    mqtt/everestmqttclient.h \
    mqtt/everestmqttdiscovery.h \
    integrationplugineverest.h
