TEMPLATE = subdirs

PLUGIN_DIRS = \
    anel                \
    aqi                 \ 
    avahimonitor        \
    awattar             \
    bluos               \
    boblight            \
    bose                \
    coinmarketcap       \
    commandlauncher     \
    datetime            \
    daylightsensor      \
    denon               \
    doorbird            \
    dweetio             \
    dynatrace           \
    elgato              \
    eq-3                \
    flowercare          \
    fronius             \
    genericelements     \
    genericthings       \
    gpio                \
    i2cdevices          \
    httpcommander       \
    keba                \
    kodi                \
    lgsmarttv           \
    mailnotification    \
    mqttclient          \
    nanoleaf            \
    netatmo             \
    networkdetector     \
    nuki                \
    onewire             \
    openuv              \ 
    openweathermap      \
    osdomotics          \
    philipshue          \
    pushbullet          \
    shelly              \
    solarlog            \
    systemmonitor       \
    remotessh           \
    senic               \
    serialportcommander \
    simulation          \
    snapd               \
    sonos               \
    tado                \
    tasmota             \
    tcpcommander        \
    texasinstruments    \
    tplink              \
    tuya                \
    udpcommander        \
    unifi               \
    usbrelay            \
    wakeonlan           \
    wemo                \
    ws2812fx            \


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

# For Qt-Creator's code model: Add CPATH to INCLUDEPATH explicitly
INCLUDEPATH += $$(CPATH)

# Verify if building only a selection of plugins
contains(CONFIG, selection) {
    # Check each plugin if the subdir exists
    for(plugin, PLUGINS) {
        contains(PLUGIN_DIRS, $${plugin}) {
            SUBDIRS*= $${plugin}
        } else {
            error("Invalid plugin passed. There is no subdirectory with the name $${plugin}.")
        }
    }
    message("Building plugin selection: $${SUBDIRS}")
} else {
    SUBDIRS *= $${PLUGIN_DIRS}
    message("Building all plugins")
}

