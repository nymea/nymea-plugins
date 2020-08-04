# ws281xspi
--------------------------------

This plugin provides an interface to connect a nymea system with
a WS281x LED strips. It uses 
[jgarff's rpi_ws281x library](https://github.com/jgarff/rpi_ws281x)
to communicate with the hardware. See the README.md file of this
library for more details on the hardware. The plugin only implements
the serial peripheral interface (SPI) connection.

## Parameters

At present, this plugin interface provides access to the following
parameters:

_Parameters:_ (can only set once when creating the thing)
* SPI Pin (the GPIO pin of the Raspberry Pi to be used for SPI output)

Parameter | Default | Allowed Values | Description
----------|---------|----------------|-----------------------------------
SPI Pin   | 10      | 10             | The GPIO pin of the Raspberry 
          |         |                | Pi to be used for SPI output. 
          |         |                | Since only pin 10 is present on 
          |         |                | all Raspberry Pi hardware, this 
          |         |                | is the only one supported for now.
Length    | 8       | > 0            | The number of LEDs that shall be 
          |         |                | accessed

_States:_

Parameter | Default | Allowed Values | Description
----------|---------|----------------|------------------------------
Brightness| 255     | 0-255          | LED brightness
Color     | 8       | > 0            | LED color
Power     | 0       | 0/1            | On/off switch


## Building the plugin

_Requirements:_

Follow the instructions 
[here](https://nymea.io/documentation/developers/build-env) to
setup the build environment. In addition, the scons software
package is required.

_Get the source code:_

The plugin can be obtained as part of the nymea-plugins repository from 
[here](https://github.com/nymea/nymea-plugins.git):

```
git checkout https://github.com/nymea/nymea-plugins.git
```

The plugin requires the rpi_ws281x library, available at Github 
[here](https://github.com/jgarff/rpi_ws281x). This is included as
git submodule with this plugin. Change to the plugin root directory
and load it as follows:

```
git submodule init
git submodule update
```

_Build the plugin:_

Create a build directory and change to it. Run qmake, where 
PATH_TO_PLUGIN_ROOT should point to the directory holding the
ws281xspi.pro file.

```
qmake PATH_TO_PLUGIN_ROOT
make
sudo make install
```

Restart your nymea:core. The plugin should be listed. Finally, add a
thing and set the parameters according to your preferences.

