The nymea plugin for Garadget Garage Door Opener requires Garadget version 1.20 or later 
nymea supports the Garadget device via the mqtt only mode using the nymea internal MQTT broker. External MQTT broker is not supported.
    Make sure nymeas MQTT broker is enabled in System Settings -> MQTT broker and create a policy (Client ID, username and password) for the garadget device to log in.
    
Note: The garadget must be configured with the MQTT mode only! otherwise the Particle operation will probably conflict.

See the (https://www.garadget.com/setup-instructions/) for garadget general setup and (https://community.garadget.com/t/mqtt-support/3226/5) for MQTT instructions

Garadget Initial Configuration:
    Put your device into listening mode: press and hold “M” button for about 3 seconds until LED starts blinking dark blue.
    Connect any WiFi enabled device to PHOTON-XXXX access point.
    Open http://192.168.0.1/ in the browser 
        Confirmed in settings that your Garadget is v1.20 or later.
    at a minimum set the following:
        select your SSID
        enter SSID password
        select Only MQTT
        enter nymead Broker IP: 
        enter nymead Broker Port:
        enter nymead Broker User ID: (if needed)
        enter nymead Broker Password: (if needed)
        enter Device Topic ID: (this must be unique across all garadget devices and contain NO /s)
    select Save & Connect (The device will reconnect to your WiFi network via dhcp)

nymea configuration:
    assuming you have installed the garadget plugin:
    select garadget plugin from the ADD things page or Configure things page. 
    select Garadget (there will be an Internal MQTT client and MQTT client - Ignore them)
    Name the thing with your choice of names
    Thing Parameter -> enter the Device Topic ID: you entered in the Garadget device above.

nymea operation:
    on the Garadget window, the up icon, stop icon, and down icon cause the device to activate the garage door.
    on the Garadget detail window (uppre righthand corner), you can tune the Garadget configuration for:
        Refection Threshold
        Button Press Time
        Door Moving Time. (you should adjust to the time it takes your door to open to prevent garadget indicating stopped unnecessarily)
    on the Configure Things page for Garadget, you can tune the Garadget configuration for:
        sensor scan interval
        multi button press delay (garadget manual labels this consecutive button presses delay)
    when changed it takes a couple seconds for Garadget to respond and update the values shown in the nymea window.

notes:
    DECKO Garage Door opener requires a ~ 3.3 Volt zener diode in series of the connection between the Garadget and the DECKO
    Garadget manual indicates the range of mtt is 1000 to 12000 ms but testing on v1.24 shows it currently only accepts 5000 to 60000 ms (nymea will request other values but Garadget may reject)
    The plugin will show connected as soon as the Garadget connects to the broker.
    The plugin will detect if the device is commanded to change broker connection or topic name and therefore set state to disconnected.
    The plugin may take 1 minute to show disconnects (power down or breakage).

Issues: Garadget operating a DECKO garage door opener (may not be issue with other garage door openers)
    the Garadget can get confused on the correct relay actions to take if you hit "stop" and then either "open" or "close" when operating a DECKO 
    It is best to be in direct view of the door if you hit "stop" (the plugin image will be halfway up) to be sure you know where the door will go next.
    This confusion is an issue of the Garadget device and not of nymea-plugin.

    If nymead reboots rapidly, Garadget may not detect and show disconnected. In this case You may need to push the reset button on the Garadget
