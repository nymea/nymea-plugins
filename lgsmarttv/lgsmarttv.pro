include(../plugins.pri)

QT += network

greaterThan(QT_MAJOR_VERSION, 5) {
    QT += core5compat
} else {
    QT += xml
}

SOURCES += \
    integrationpluginlgsmarttv.cpp \
    tvdevice.cpp \
    tveventhandler.cpp

HEADERS += \
    integrationpluginlgsmarttv.h \
    tvdevice.h \
    tveventhandler.h


