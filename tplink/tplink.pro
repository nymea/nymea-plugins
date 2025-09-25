include(../plugins.pri)

QT += network

SOURCES += \
    integrationplugintplink.cpp

HEADERS += \
    integrationplugintplink.h

OTHER_FILES += \
    sampledata/HS200.txt \
    sampledata/HS300-UDP.txt

DISTFILES += \
    sampledata/HS300-TCP.txt
