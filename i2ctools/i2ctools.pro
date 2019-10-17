include(../plugins.pri)

TARGET = $$qtLibraryTarget(nymea_deviceplugini2ctools)

LIBS += -li2c \

SOURCES += \
    deviceplugini2ctools.cpp \
    i2cbusses.c

HEADERS += \
    deviceplugini2ctools.h \
    i2cbusses.h


