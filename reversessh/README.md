# Reverse ssh

This plugin allows to establish a reverse SSH tunnel to the device where nymea is running.

This is useful when maintaining remote nymea setups which may be hidden behind a firewall
and cannot be accessed from the public internet. A user can easily enable SSH access for
a nymea setup by adding a Thing using the app without having to deal with DNS and NAT.

## Requirements

In order to establish a reverse SSH tunnel, a SSH server is required to be accessible from
both, the nymea instance and the client log in from. Also a SSH server is required
to run on the system where nymea is running (If using the nymea images, this is already the case).

## Setup

### SSH Server setup

The SSH server can be hosted anywhere, for instance on a vserver somewhere in the internet.
The following settings must be enabled on the SSH server for it to work (assuming openssh):

* AllowTcpForwarding yes (To allow this sort of forwarding generally)
* GatewayPorts yes (To allow reverse ssh from other hosts than the reverse proxy itself - it would listen to localhost only otherwise)

Create a user on the SSH server. Note that if sharing the credentials with someone else it
might be advisable to confine the SSH server in a container, however, such a setup is beyond
the scope of this manual. This will assume that both ends are trusted and SSH credentials
for the server can be shared.

### Nymea setup

During the thing setup, enter the server connection information for SSH server:

* SSH server address: The hostname or IP of the SSH server
* SSH server port: The SSH port of the SSH server (22 by default)
* Local SSH server port: The SSH port of the local SSH server running on the nymea system (22 by default)
* Remote port to be opened: This is the port on which the nymea system will be reachable. This can be any port number which isn't in use already, and, unless you intend to log in as root (not advisable) this must be a port higher than 1024

In the next step, provide the SSH credentials for the user on the SSH server which has been created before. Once the login succeeds, the thing should become connected.

### Connecting with an SSH client

Once the above setup succeeded, the nymea system can be reached via SSH using:

    ssh <user>@<server> -p <remote open port>

where `user` is a user on the nymea system, server is the IP or hostname of the SSH server and `remote open port` is the port that has been picked during the thing setup.

## More

https://www.howtogeek.com/428413/what-is-reverse-ssh-tunneling-and-how-to-use-it/
