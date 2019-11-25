include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_deviceplugini2ctools)

LIBS += -li2c \

SOURCES += \
    deviceplugini2ctools.cpp \

HEADERS += \
    deviceplugini2ctools.h \


