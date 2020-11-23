# TCP commander

This plugin is a generic approach to allow sending and receiving custom TCP packages.

> Note: This plugin is ment to be combined with a rule.

## TCP client

The TCP output opens a TCP connection to the given host address and port. Once the connection is established, TCP packets can be sent both directions.

## TCP server

The TCP input creates a TCP server on the given port. Other applications may connect to this server and send messages to it which can be processed further within nymea. Also, TCP packets can be sent to all or individual clients. Use the address 0.0.0.0 (the default) to send the data to all connected clients.

## Example

If you create a TCP Input on port 2323 and with the command `"Light 1 ON"`, following command will trigger an event in nymea and allows you to connect this event with a rule.

> Note: In this example nymea is running on `localhost`

    $ echo "Light 1 ON" | nc localhost 2323
    OK

If you create a TCP output on port 2324 and IP address 127.0.0.1, send in nymea the command `"Light 1 is ON"` and Netcat (nc) will receive and display your command.

> Note: the command is running on `localhost` 

    $ while :; do nc -l -p 2324; sleep 1; done
    Light 1 is ON
