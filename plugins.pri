TEMPLATE = lib
CONFIG += plugin

QT += network bluetooth

QMAKE_CXXFLAGS += -Werror -std=c++11 -g
QMAKE_LFLAGS += -std=c++11

INCLUDEPATH += /usr/include/nymea
LIBS += -lnymea
HEADERS += $${OUT_PWD}/plugininfo.h


PLUGIN_PATH=/usr/lib/$$system('dpkg-architecture -q DEB_HOST_MULTIARCH')/nymea/plugins/

# Check if this is a snap build
snappy{
    INCLUDEPATH+=$$(SNAPCRAFT_STAGE)/usr/include/nymea
}

# Make the device plugin json file visible in the Qt Creator
OTHER_FILES+=$$PWD/$${TARGET}/deviceplugin"$$TARGET".json

# Create plugininfo file
plugininfo.commands = nymea-generateplugininfo --filetype i --jsonfile $$PWD/$${TARGET}/deviceplugin"$$TARGET".json --output plugininfo.h --builddir $$OUT_PWD; \
                      nymea-generateplugininfo --filetype e --jsonfile $$PWD/$${TARGET}/deviceplugin"$$TARGET".json --output extern-plugininfo.h --builddir $$OUT_PWD;
plugininfo.depends = FORCE
QMAKE_EXTRA_TARGETS += plugininfo
PRE_TARGETDEPS += plugininfo

# Install translation files
TRANSLATIONS *= $$files($${PWD}/$${TARGET}/translations/*ts, true)
lupdate.depends = FORCE
lupdate.depends += plugininfo
lupdate.commands = lupdate -recursive -no-obsolete $$PWD/"$$TARGET"/"$$TARGET".pro;
QMAKE_EXTRA_TARGETS += lupdate

translations.path = /usr/share/nymea/translations
translations.files = $$[QT_SOURCE_TREE]/translations/*.qm

# Install plugin
target.path = $$PLUGIN_PATH
target.depends += plugininfo
INSTALLS += target translations

