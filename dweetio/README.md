# Dweetio

Connect nymea to dweet.io.

Using this plugin you can post messages to dweet.io from nymea or subscribe nymea to dweets from other IoT devices.

To post data from nymea set up a new "Post" thing using the thing name of your choice, then create a rules in nymea that call your thing's post action.

To follow to other dweets, create a new "Get" thing with the thing name you'd like to follow. It will produce events in nymea which can be used to launch other things, or just get logged in nymea's log database.
