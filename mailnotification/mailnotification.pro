include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginmailnotification)

QT+= network

SOURCES += \
    devicepluginmailnotification.cpp \
    smtpclient.cpp

HEADERS += \
    devicepluginmailnotification.h \
    smtpclient.h


