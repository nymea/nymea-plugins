TEMPLATE = subdirs

PLUGINS_DIRS = \
    elro                \
    intertechno         \
    networkdetector     \
    conrad              \
    openweathermap      \
    lircd               \
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
    kodi                \
    elgato              \
    awattar             \
    netatmo             \
    dollhouse           \
    plantcare           \
    osdomotics          \
    ws2812              \
    orderbutton         \
    denon               \
    avahimonitor        \
    pushbullet          \
    usbwde              \
    senic               \
    multisensor         \
    gpio                \


CONFIG+=all

message(============================================)
message("Qt version:" $$[QT_VERSION])

# Verify if building only a selection of plugins
contains(CONFIG, selection) {
    CONFIG-=all

    # Check each plugin if the subdir exists
    for(plugin, PLUGINS) {
        contains(PLUGINS_DIRS, $${plugin}) {
            SUBDIRS*= $${plugin}
        } else {
            error("Invalid plugin passed. There is no subdirectory with the name $${plugin}.")
        }
    }
    message("Building plugin selection: $${SUBDIRS}")
}

all {
    SUBDIRS *= $${PLUGINS_DIRS}
    message("Building all plugins")
}

