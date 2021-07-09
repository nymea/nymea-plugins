# Importing this file will set up most needed things for python plugins.
# IMPORTANT: This file needs to be included after defining the plugin SOURCES
#
# Example python plugin .pro file:
#
# PY_SOURCES += integrationpluginexample.py
# include(../python-plugins.pri)

TEMPLATE = aux

JSONFILE=$${_PRO_FILE_PWD_}/integrationplugin"$$TARGET".json
REQUIREMENTSFILE=$${_PRO_FILE_PWD_}/requirements.txt

# Make the device plugin json file visible in the Qt Creator
OTHER_FILES *= $$JSONFILE $$PY_SOURCES

# Add a make requirements target to fetch requirements
requirements.commands = pip3 install -r $${_PRO_FILE_PWD_}/requirements.txt -t $${_PRO_FILE_PWD_}/modules/
QMAKE_EXTRA_TARGETS += requirements


# Install plugin
target.path = $$[QT_INSTALL_LIBS]/nymea/plugins/$${TARGET}
target.files += $${OTHER_FILES}

modules.path = $$[QT_INSTALL_LIBS]/nymea/plugins/$${TARGET}
modules.files = $${_PRO_FILE_PWD_}/modules/
modules.depends += requirements

INSTALLS += target modules

