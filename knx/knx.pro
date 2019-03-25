# Note: make external qtknx linking
exists(/opt/qtknx/build/mkspecs/modules/qt_lib_knx.pri) {
    include(/opt/qtknx/build/mkspecs/modules/qt_lib_knx.pri)
}

include(../plugins.pri)

# Note: requires qtknx
QT += network knx

TARGET = $$qtLibraryTarget(nymea_devicepluginknx)

SOURCES += \
    devicepluginknx.cpp \
    knxtunnel.cpp \
    knxserverdiscovery.cpp

HEADERS += \
    devicepluginknx.h \
    knxtunnel.h \
    knxserverdiscovery.h

