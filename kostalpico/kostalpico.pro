include($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri)

QT += network
QT += core

QMAKE_CXXFLAGS_WARN_ON += -Wno-reorder

SOURCES += \
    integrationpluginkostalpico.cpp \
    kostalnetworkreply.cpp \
    kostalpicoconnection.cpp

HEADERS += \
    integrationpluginkostalpico.h \
    kostalnetworkreply.h \
    kostalpicoconnection.h

