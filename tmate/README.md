# Tmate

This plugin allows to establish a terminal connection to the device where nymea is running.

This is useful when maintaining remote nymea setups which may be hidden behind a firewall
and cannot be accessed from the public internet. A user can easily enable SSH access for
a nymea setup by adding a Thing using the app without having to deal with DNS and NAT.

## Setup

A tmate thing can be created without any additional information. Leaving all the fields
empty during setup will establish a default tmate session to tmate.io servers and generate
a new random token. This is the equivalent of just running `tmate` in a terminal.

For a more permanent solution, an API key can be registered at tmate.io. Using the api key a custom session name can be set which will be used for every connection establishment, persisting across nymea restarts. In the 
next step, provide the SSH credentials for the user on the SSH server which has been created before. Once the login succeeds, the thing should become connected.

> Note: Using custom/self hosted tmate servers is not supported at this point.

### Connecting clients

The thing will inform about the session names via states and can be accessed via ssh or web.

## More

https://tmate.io/
