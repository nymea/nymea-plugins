include(../plugins.pri)

QT += network

LIBS += -lzbar

TARGET = $$qtLibraryTarget(nymea_deviceplugindoorbird)

SOURCES += \
    deviceplugindoorbird.cpp \
    doorbird.cpp \
    qrcodereader.cpp \

HEADERS += \
    deviceplugindoorbird.h \
    doorbird.h \
    qrcodereader.h \
