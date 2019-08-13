# TCP commander

This plugin is a generic approach to allow sending and receiving custom TCP packages.

> Note: This plugin is ment to be combined with a rule.

## TCP output

The TCP output opens a TCP connection to the given host IPv4 address and port everytime the output trigger gets activated. As soon 
as the command has been executed the socket will close again. The connected state is only stated as connected 
as long as the connection is active.

## TCP input

The TCP input creates a TCP server on the given port.

Be aware that only a single connection can be established simultaneously. The connected state is active as long as
a client is connected. It is up to the client to deside how long the connection stays active.

## Example

If you create a TCP Input on port 2323 and with the command `"Light 1 ON"`, following command will trigger an event in nymea and allows you to connect this event with a rule.

> Note: In this example nymea is running on `localhost`

    $ echo "Light 1 ON" | nc localhost 2323
    OK

If you create a TCP output on port 2324 and IP address 127.0.0.1, send in nymea the command `"Light 1 is ON"` and Netcat (nc) will receive and display your command.

> Note: the command is running on `localhost` 

    $ while :; do nc -l -p 2324; sleep 1; done
    Light 1 is ON
