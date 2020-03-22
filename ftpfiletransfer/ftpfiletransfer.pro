include(../plugins.pri)

QT += network

TARGET = $$qtLibraryTarget(nymea_integrationpluginftpfiletransfer)

SOURCES += \
    integrationpluginftpfiletransfer.cpp \
    filesystem.cpp \
    ftpupload.cpp

HEADERS += \
    integrationpluginftpfiletransfer.h \
    filesystem.h \
    ftpupload.h
