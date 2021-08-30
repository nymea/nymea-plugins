include(../plugins.pri)

QT += bluetooth

TARGET = $$qtLibraryTarget(nymea_integrationpluginlukeroberts)

SOURCES += \
    integrationpluginlukeroberts.cpp \
    lukeroberts.cpp

HEADERS += \
    integrationpluginlukeroberts.h \
    lukeroberts.h


