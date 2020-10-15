include(../plugins.pri)

QT += \
    network \

SOURCES += \
    integrationpluginfronius.cpp \
    froniusthing.cpp \
    froniuslogger.cpp \
    froniusinverter.cpp \
    froniusstorage.cpp \
    froniusmeter.cpp \

HEADERS += \
    integrationpluginfronius.h \
    froniusthing.h \
    froniuslogger.h \
    froniusinverter.h \
    froniusstorage.h \
    froniusmeter.h \
