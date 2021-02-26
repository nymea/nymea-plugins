# Telegram

This plugin allows to send message to Telegram via the Telegram bot API.

https://core.telegram.org/bots/api

## Usage

In order to use the telegram plugin, a new bot must be created and the bot access token provided to nymea.

Instructions to create a telegram bot:

* Open a conversation to @BotFather

* Send the message `/newbot` to BotFather and follow the instructions until BotFather hands out the token for your bot.

* Now open a conversation to your bot and write it a message. The content of this message does not matter but it is important that at least one message has been sent to the bot in order for it to work.

* Alternatively, add the bot to a group chat and send a message to the group mentioning the bot with @<bot_name>. The content of this message does not matter as long as it contains a mention for the bot. This is required for the bot to see the message and allow nymea recognizing the group.

* Now return to nymea and set up a new telegram thing as usual, providing the token during the setup.
