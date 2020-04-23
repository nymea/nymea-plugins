include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_integrationplugini2ctools)

LIBS += -li2c \

SOURCES += \
    integrationplugini2ctools.cpp \

HEADERS += \
    integrationplugini2ctools.h \


