include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_deviceplugintcpcommander)

SOURCES += \
    deviceplugintcpcommander.cpp \
    tcpserver.cpp \
    tcpsocket.cpp

HEADERS += \
    deviceplugintcpcommander.h \
    tcpserver.h \
    tcpsocket.h
