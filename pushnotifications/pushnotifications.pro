include(../plugins.pri)

QT *= network

# For RSA signing JWT
PKGCONFIG += openssl

SOURCES += \
    googleoauth2.cpp \
    integrationpluginpushnotifications.cpp

HEADERS += \
    googleoauth2.h \
    integrationpluginpushnotifications.h


