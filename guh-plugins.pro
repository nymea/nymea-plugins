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
    pushbullet          \
    usbwde              \
    multisensor         \
    gpio                \
    snapd               \


CONFIG+=all

message(============================================)
message("Qt version:" $$[QT_VERSION])

# Translations:
# make lupdate to update .ts files
lupdate.depends = FORCE
for (entry, PLUGIN_DIRS):lupdate.commands += make -C $${entry} lupdate;
QMAKE_EXTRA_TARGETS += lupdate

# make lrelease to build .qm from .ts
lrelease.depends = FORCE
for (entry, PLUGIN_DIRS):lrelease.commands += $$[QT_INSTALL_BINS]/lrelease $$files($$PWD/$${entry}/translations/*.ts, true);
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

