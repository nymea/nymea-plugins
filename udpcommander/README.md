# UDP commander

This plugin allows to receive UDP packages over a certain UDP port and generates an event if the message content matches the param command.

> Note: This plugin is ment to be combined with a rule.

## Example

If you create an UDP Commander on port 2323 and with the command `"Light 1 ON"`, following command will trigger an event in nymea and allows you to connect this event with a rule.

> Note: In this example nymea is running on `localhost`


    $ echo "Light 1 ON" | nc -u localhost 2323
    OK

This allows you to execute actions in your nymea system when a certain UDP message will be sent to nymea.

If the command will be recognized from nymea, the sender will receive as answere a `"OK"` string.
