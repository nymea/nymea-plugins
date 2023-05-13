# Owlet

nymea:owlet is a firmware for different microcontrollers (Arduino/ESP8266/ESP32) which
exposes pins of the microcontroller to nymea and allows using them for
whatever purpose like moodlights, control relays, reading analog values
or controlling servos.

# ESP8285

# ESP32

# M5Stick-C


# Arduino

In order to use owlet with an [Arduino](https://docs.arduino.cc/) you need to add the corresponding model into nymea. Once the thing is connected the firmware will be flashed automatically if required.

Following models are available:

* [Arduino Uno](https://docs.arduino.cc/hardware/uno-rev3)
* [Arduino Mini Pro (5v, 16MHz)](https://docs.arduino.cc/retired/boards/arduino-pro-mini)
* [Arduino Mini Pro (3.3v, 8MHz)](https://docs.arduino.cc/retired/boards/arduino-pro-mini)
* [Arduino Nano](https://store.arduino.cc/products/arduino-nano)

Once the Arduino has been added to nymea and the owlet firmware has been flashed successfully, the pins
can be configured as desired within the Arduino thing settings. Depending on the pin capabilities you can select
how the mode for each pin you require for your project. By default all pins are unconfigured.

When applying the settings, for each pin a new thing will appear in nymea giving you controls and information about
the current states of the pin. If a pin has been configured to a specific type and you want to remove a thing,
just configure it as type `None` and the associated thing will be removed from nymea.


## Available configurations

Not all pins can be configured to any type. Within the settings you can see for each pin
the possible configurations.

### Digital Output

If you configure a pin as *Output* you can switch on and off the associated GPIO.

Usecase examples:

* Switching relays
* Enable / Disable a LED

### Digital Input

If you configure a pin as *Input* you get a thing with a `bool` state indicating if the current state of the pin.

Usecase examples:

* Buttons
* Contact sensors

### Analog output (PWM)

If you configure a pin as *PWM* you can set the current duty cylcle of the PWM in a range of 0 - 255. The frequency of the duty cycle depends on the hardware you are using and requires some datasheet reading in order to understand hot it works.

[Arduino analogWrite() refference](https://www.arduino.cc/reference/en/language/functions/analog-io/analogwrite/
)

Usecase examples:

* LED brightness
* Piezo buzzer
* Control motor speed

### Analog input (ADC)

If you configure a pin as *Analog input* you can read periodically the value from the internal ADC (analog digital converter). Most ADCs have a resolution of 10 Bits giving you a value range of 0 - 1023. Once you configured the pin as analog input you can configure in the thing settings how often the value should be fetched. Default is every 500 ms.

Usecase examples:

* Reading analog sensor values like humidity, distance...
* Reading a potentiometer value

### Servo

If you configure a pin as *Servo* you can control a servo motor in the range of 0° - 180°. A servo normally gets controlled using a PWM signal, but for most DIY servos the internal PWM functionality has a to high frequency. This mode makes use of the [Arduino Servo library](https://www.arduino.cc/reference/en/libraries/servo/) and uses the internal timers to generate a customizable PWM signal.

> Please note that if you configure a Servo, 2 internal timers will be used and therefore you loose some native PWM functionality. Plase read the documetation of your Hardware.
