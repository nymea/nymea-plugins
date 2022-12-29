# Notify.Events

This plugin allows to send messages to the Notify.Events service which in turn allows to distribute
the messages to various receivers, such as instant messengers, email, push notifications and more.

## Requirements

First go to Notify.Events and sign up. Create a new channel. In the channel, add a new message source and
select nymea. Copy the token.
Set up a new Notify.Events thing in nymea, providing the token for the channel. Multiple things may be set
up to send messages to different channels.

At this point nymea can send messages to Notify.Events which will be redistributed to all receivers configured
on Notify.Events for this channel.
