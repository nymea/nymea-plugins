include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_devicepluginftpfiletransfer)

SOURCES += \
    devicepluginftpfiletransfer.cpp \
    filesystem.cpp \
    ftpupload.cpp

HEADERS += \
    devicepluginftpfiletransfer.h \
    filesystem.h \
    ftpupload.h
