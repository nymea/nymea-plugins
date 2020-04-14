include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationplugintcpcommander)

SOURCES += \
    integrationplugintcpcommander.cpp \
    tcpserver.cpp \
    tcpsocket.cpp

HEADERS += \
    integrationplugintcpcommander.h \
    tcpserver.h \
    tcpsocket.h
