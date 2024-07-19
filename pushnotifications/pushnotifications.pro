include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationpluginpushnotifications)

QT+= network

# For RSA signing JWT
PKGCONFIG += openssl

SOURCES += \
    googleoauth2.cpp \
    integrationpluginpushnotifications.cpp

HEADERS += \
    googleoauth2.h \
    integrationpluginpushnotifications.h


