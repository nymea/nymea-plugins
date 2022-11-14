include(../plugins.pri)

PKGCONFIG += nymea-sunspec

QT += network serialbus

SOURCES += \
    froniusnetworkreply.cpp \
    froniussolarconnection.cpp \
    integrationpluginfronius.cpp \

HEADERS += \
    froniusnetworkreply.h \
    froniussolarconnection.h \
    integrationpluginfronius.h \
