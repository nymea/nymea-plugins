# ZigBee Aqara / Xiaomi Mi / Lumi

This plugin allows to interact with ZigBee from Aqara / Xiaomi Mi / Lumi using a native ZigBee network controller in nymea.

## Supported Things

In order to bring a ZigBee device into the nymea ZigBee network, the network needs to be opened for joining before you perform the device pairing instructions. The joining process can take up to 30 seconds. If the device does not show up, please restart the pairing process.

> Note: if you you already added one of the following devices and you restart the pairing process while the network is closed, the device will leave the ZigBee network and the thing will be removed automatically.

### Aqara smart plug

The [Aqara smart plug](https://www.aqara.com/en/smart_plug.html) can be switched on and off. The power consumtion is not suppoted yet, but planed for the future.


**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button until the LED starts blinking.


### Aqara weather sensor

The [Aqara weather sensor](https://www.aqara.com/us/temperature_humidity_sensor.html) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button until the LED starts blinking.


### Aqara door and window sensor

The [Aqara door and window sensor](https://www.aqara.com/us/door_and_window_sensor.html) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button until the LED starts blinking.


### Aqara motion sensor

The [Aqara motion sensor](https://www.aqara.com/us/motion_sensor.html) is fully supported. The duration for the present state can be configured in the thing settings.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button until the LED starts blinking.


### Aqara water leak sensor

The [Aqara water leak sensor](https://www.aqara.com/us/water_leak_sensor.html) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button until the LED starts blinking.


### Aqara vibration sensor

The [Aqara vibration sensor](https://www.aqara.com/us/vibration_sensor.html) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button until the LED starts blinking.


### Aqara mini switch

The [Aqara mini switch](https://www.aqara.com/us/wireless_mini_switch.html) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button until the LED starts blinking.


### Aqara wall switch (Double key)

The [Aqara wall switch (Double key)](https://xiaomi-mi.com/sockets-and-sensors/remote-switch-for-aqara-smart-light-wall-switch-double-key/) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button until the LED starts blinking.


### Aqara single switch Module T1

The [Aqara single switch module T1](https://www.aqara.com/eu/single_switch_T1_no-neutral.html) is fully supported.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button until the LED starts blinking.


### Xiaomi Mi temperature humidity sensor

The [Aqara single switch module T1](https://xiaomi-mi.com/sockets-and-sensors/xiaomi-mi-temperature-humidity-sensor/) is fully supported.

> Note: it takes about 30 seconds until the sensor shows up because the initilization takes longer than normal.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button with a needle until the LED starts blinking.


### Xiaomi Mi wireless switch

The [Xiaomi Mi wireless switch](https://xiaomi-mi.com/sockets-and-sensors/xiaomi-mi-wireless-switch/) supports currently only the single button press. The multi click events will be implemented in the future.

> Note: it takes about 30 seconds until the sensor shows up because the initilization takes longer than normal.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button with a needle until the LED starts blinking.

### Xiaomi Mi door and window sensor

The [Xiaomi Mi door and window sensor](https://xiaomi-mi.com/sockets-and-sensors/xiaomi-mi-door-window-sensors/) is fully supported.

> Note: it takes about 30 seconds until the sensor shows up because the initilization takes longer than normal.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button with a needle until the LED starts blinking.

### Xiaomi Mi occupancy sensor

The [Xiaomi Mi occupancy sensor](https://xiaomi-mi.com/sockets-and-sensors/xiaomi-mi-occupancy-sensor/) is fully supported. The duration for the present state can be configured in the thing settings.

> Note: it takes about 30 seconds until the sensor shows up because the initilization takes longer than normal.

**Pairing instructions**: Open the ZigBee network for joining. Press and hold the pairing button with a needle until the LED starts blinking.


## Requirements

* A compatible ZigBee controller and a running ZigBee network in nymea. You can find more information about supported controllers and ZigBee network configurations [here](https://nymea.io/documentation/users/usage/configuration#zigbee).

