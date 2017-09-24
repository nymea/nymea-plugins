TRANSLATIONS = translations/en_US.ts \
               translations/de_DE.ts

# Note: include after the TRANSLATIONS definition
include(../plugins.pri)

TARGET = $$qtLibraryTarget(guh_deviceplugintcpcommander)

SOURCES += \
    deviceplugintcpcommander.cpp \
    tcpserver.cpp \

HEADERS += \
    deviceplugintcpcommander.h \
    tcpserver.h \

