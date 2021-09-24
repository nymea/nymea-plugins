include($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri)

QT += network

PKGCONFIG += nymea-mqtt

TARGET = $$qtLibraryTarget(nymea_integrationpluginGaradget)

SOURCES += \
    integrationpluginGaradget.cpp

HEADERS += \
    integrationpluginGaradget.h

