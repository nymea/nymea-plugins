include(../plugins.pri)

TARGET = $$qtLibraryTarget(guh_deviceplugintcpcommander)

SOURCES += \
    deviceplugintcpcommander.cpp \
    tcpserver.cpp \

HEADERS += \
    deviceplugintcpcommander.h \
    tcpserver.h \

