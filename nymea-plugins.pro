TEMPLATE = subdirs

PLUGIN_DIRS = \
    elro                \
    intertechno         \
    networkdetector     \
    conrad              \
    openweathermap      \
    wakeonlan           \
    mailnotification    \
    philipshue          \
    eq-3                \
    wemo                \
    lgsmarttv           \
    datetime            \
    genericelements     \
    commandlauncher     \
    unitec              \
    leynew              \
    udpcommander        \
    tcpcommander        \
    httpcommander       \
    kodi                \
    elgato              \
    senic               \
    awattar             \
    netatmo             \
    plantcare           \
    osdomotics          \
    ws2812              \
    orderbutton         \
    denon               \
    avahimonitor        \
    gpio                \
    snapd               \
    simulation          \
    keba                \
    remotessh           \
    dweetio             \ 
    flowercare          \

CONFIG+=all

message(============================================)
message("Qt version:" $$[QT_VERSION])

plugininfo.depends = FORCE
for (entry, PLUGIN_DIRS):plugininfo.commands += test -d $${entry} || mkdir -p $${entry}; cd $${entry} && qmake -o Makefile $$PWD/$${entry}/$${entry}.pro && cd ..;
for (entry, PLUGIN_DIRS):plugininfo.commands += make -C $${entry} plugininfo.h;
QMAKE_EXTRA_TARGETS += plugininfo

# Translations:
# make lupdate to update .ts files
lupdate.depends = FORCE plugininfo
for (entry, PLUGIN_DIRS):lupdate.commands += make -C $${entry} lupdate;
QMAKE_EXTRA_TARGETS += lupdate

# make lrelease to build .qm from .ts
lrelease.depends = FORCE
for (entry, PLUGIN_DIRS):lrelease.commands += lrelease $$files($$PWD/$${entry}/translations/*.ts, true);
for (entry, PLUGIN_DIRS):lrelease.commands += rsync -a $$PWD/$${entry}/translations/*.qm $$OUT_PWD/translations/;
QMAKE_EXTRA_TARGETS += lrelease

# Verify if building only a selection of plugins
contains(CONFIG, selection) {
    CONFIG-=all

    # Check each plugin if the subdir exists
    for(plugin, PLUGINS) {
        contains(PLUGIN_DIRS, $${plugin}) {
            SUBDIRS*= $${plugin}
        } else {
            error("Invalid plugin passed. There is no subdirectory with the name $${plugin}.")
        }
    }
    message("Building plugin selection: $${SUBDIRS}")
}

all {
    SUBDIRS *= $${PLUGIN_DIRS}
    message("Building all plugins")
}

