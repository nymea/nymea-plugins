TEMPLATE = lib
CONFIG += plugin

QT += network bluetooth

QMAKE_CXXFLAGS += -Werror -std=c++11 -g
QMAKE_LFLAGS += -std=c++11

INCLUDEPATH += /usr/include/guh
LIBS += -lguh
HEADERS += $${OUT_PWD}/plugininfo.h


PLUGIN_PATH=/usr/lib/$$system('dpkg-architecture -q DEB_HOST_MULTIARCH')/guh/plugins/

# Check if this is a snap build
snappy{
    INCLUDEPATH+=$$(SNAPCRAFT_STAGE)/usr/include/guh
}

# Create plugininfo file
JSONFILES = deviceplugin"$$TARGET".json
plugininfo.target = plugininfo.h
plugininfo.output = plugininfo.h
plugininfo.CONFIG = no_link
plugininfo.input = JSONFILES
plugininfo.commands = touch ${QMAKE_FILE_OUT}; guh-generateplugininfo \
                            --filetype i \
                            --jsonfile ${QMAKE_FILE_NAME} \
                            --output ${QMAKE_FILE_OUT} \
                            --builddir $$OUT_PWD;
PRE_TARGETDEPS += compiler_plugininfo_make_all
QMAKE_EXTRA_COMPILERS += plugininfo

externplugininfo.target = extern-plugininfo.h
externplugininfo.output = extern-plugininfo.h
externplugininfo.CONFIG = no_link
externplugininfo.input = JSONFILES
externplugininfo.commands = touch ${QMAKE_FILE_OUT}; guh-generateplugininfo \
                            --filetype e \
                            --jsonfile ${QMAKE_FILE_NAME} \
                            --output ${QMAKE_FILE_OUT} \
                            --builddir $$OUT_PWD;
PRE_TARGETDEPS += compiler_externplugininfo_make_all
QMAKE_EXTRA_COMPILERS += externplugininfo

# Install translation files
TRANSLATIONS *= $$files($${PWD}/$${TARGET}/translations/*ts, true)
lupdate.depends = FORCE
lupdate.depends += plugininfo
lupdate.commands = lupdate -recursive -no-obsolete $$PWD/"$$TARGET"/"$$TARGET".pro;
QMAKE_EXTRA_TARGETS += lupdate

translations.path = /usr/share/guh/translations
translations.files = $$[QT_SOURCE_TREE]/translations/*.qm

# Install plugin
target.path = $$PLUGIN_PATH
INSTALLS += target translations

