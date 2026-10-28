TEMPLATE = app
TARGET = tst_goecompatibility
QT = core network testlib
CONFIG += console testcase c++17 link_pkgconfig
PKGCONFIG += nymea
INCLUDEPATH += ..
SOURCES += tst_goecompatibility.cpp
