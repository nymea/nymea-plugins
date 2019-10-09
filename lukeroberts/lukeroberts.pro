include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_devicepluginlukeroberts)

SOURCES += \
    devicepluginlukeroberts.cpp \
    lukeroberts.cpp

HEADERS += \
    devicepluginlukeroberts.h \
    lukeroberts.h


