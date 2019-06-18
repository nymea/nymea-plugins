# MQTT client

This plugin allows to subscribe and publish to MQTT brokers (the nymea internal broker and external ones).

> This plugin is ment to be combined with a rule.


## Example

A device is configured to publish its state to a MQTT broker. Using this plugin the user can subscribe to
the same topic on that broker and monitor the device's state.

Publishing is also supported. This allows use cases such as controlling IoT things via MQTT by publishing
to topics such devices are subscribed to. Other possibilities are to use nymea as a "translator" between other
transport layers to MQTT. For instance a sensor might deliver sensor data via Bluetooth to Nymea and using this
MQTT plugin and a rule nymea can be configured to forward all those sensor values to a MQTT broker.
