include($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri)

# Build rpi_ws281x library in git submodule
QMAKE_PRE_LINK = scons -C $$_PRO_FILE_PWD_/rpi_ws281x libws2811.a

# Clean rpi_ws281x library in git submodule
rpi_lib_clean.target = rpi_lib_clean
rpi_lib_clean.commands = scons -C $$_PRO_FILE_PWD_/rpi_ws281x -c

# Add this extra target as dependency for clean target
QMAKE_EXTRA_TARGETS += rpi_lib_clean
clean.depends = rpi_lib_clean

QMAKE_CXXFLAGS -= -Werror

LIBS += -L$$_PRO_FILE_PWD_/rpi_ws281x/ -lws2811
INCLUDEPATH += $$_PRO_FILE_PWD_/rpi_ws281x

SOURCES += \
    integrationpluginws281xspi.cpp \
    ws281xspicontroller.cpp

HEADERS += \
    integrationpluginws281xspi.h \
    ws281xspicontroller.h

