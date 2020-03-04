include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginmailnotification)

QT+= network

SOURCES += \
    integrationpluginmailnotification.cpp \
    smtpclient.cpp

HEADERS += \
    integrationpluginmailnotification.h \
    smtpclient.h


