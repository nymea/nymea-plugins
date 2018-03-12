TEMPLATE = lib
CONFIG += plugin

QT += network bluetooth

QMAKE_CXXFLAGS += -Werror -std=c++11 -g
QMAKE_LFLAGS += -std=c++11

INCLUDEPATH += /usr/include/nymea
LIBS += -lnymea
HEADERS += $${OUT_PWD}/plugininfo.h \
           $${OUT_PWD}/extern-plugininfo.h

PLUGIN_PATH=/usr/lib/$$system('dpkg-architecture -q DEB_HOST_MULTIARCH')/nymea/plugins/

# Check if this is a snap build
snappy{
    INCLUDEPATH+=$$(SNAPCRAFT_STAGE)/usr/include/nymea
}

# Make the device plugin json file visible in the Qt Creator
OTHER_FILES+=$$PWD/$${TARGET}/deviceplugin"$$TARGET".json


# NOTE: if the code includes "plugininfo.h", it would fail if we only give it a compiler for $$OUT_PWD/plugininfo.h
# Let's add a dummy target with the plugininfo.h file without any path to allow the developer to just include it like that.

# Create plugininfo file
plugininfo.target = $$OUT_PWD/plugininfo.h
plugininfo_dummy.target = plugininfo.h
plugininfo.depends = FORCE
plugininfo.commands = nymea-generateplugininfo --filetype i --jsonfile $$PWD/$${TARGET}/deviceplugin"$$TARGET".json --output plugininfo.h --builddir $$OUT_PWD
plugininfo_dummy.commands = $$plugininfo.commands
QMAKE_EXTRA_TARGETS += plugininfo plugininfo_dummy

# Create extern-plugininfo file
extern_plugininfo.target = $$OUT_PWD/extern-plugininfo.h
extern_plugininfo_dummy.target = extern-plugininfo.h
extern_plugininfo.depends = FORCE
extern_plugininfo.commands = nymea-generateplugininfo --filetype e --jsonfile $$PWD/$${TARGET}/deviceplugin"$$TARGET".json --output extern-plugininfo.h --builddir $$OUT_PWD
extern_plugininfo_dummy.commands = $$extern_plugininfo.commands
QMAKE_EXTRA_TARGETS += extern_plugininfo extern_plugininfo_dummy

# Install translation files
TRANSLATIONS *= $$files($${PWD}/$${TARGET}/translations/*ts, true)
lupdate.depends = FORCE
lupdate.depends += plugininfo.h
lupdate.commands = lupdate -recursive -no-obsolete $$PWD/"$$TARGET"/"$$TARGET".pro;
QMAKE_EXTRA_TARGETS += lupdate

translations.path = /usr/share/nymea/translations
translations.files = $$[QT_SOURCE_TREE]/translations/*.qm
TRANSLATIONS += $$files($$[QT_SOURCE_TREE]/translations/*.ts, true)

# Install plugin
target.path = $$PLUGIN_PATH
INSTALLS += target translations

