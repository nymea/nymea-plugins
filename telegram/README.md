# Telegram

This plugin allows to send message to Telegram via the Telegram bot API.

https://core.telegram.org/bots/api

## Usage

In order to use the telegram plugin, a new bot must be created and the bot access token provided to nymea.

Instructions to create a telegram bot:

### Step 1: Creating a bot for nymea

* Open a conversation to @BotFather
* Send the message `/newbot` to BotFather and follow the instructions until BotFather hands out the token for your bot.

### Step 2: Let the bot know of your chats
Depending on whether you want the bot to send messages to a single user or to a group chat, the next step differs. It is also possible to have both of them with the same bot.

#### Option 1: Sending private messages to a Telegram user

* Open a conversation to your bot and write it a message. The content of this message does not matter but it is important that at least one message has been sent to the bot from every user you want to send messages to.

#### Option 2: Sending messages to Telegram group chats

* Make sure Bot groupy privacy is turned off for your bot to receive group messages. Go to @BotFather, send `/mybots`, and find your way to turn group privacy off for your bot.
* Add the bot to a group chat and send a message to the group.

### Step 3: Adding the bot to nymea

* Now return to nymea and set up a new telegram thing. Nymea will ask you for the bot token in the first step. Paste the token and press next. Nymea should now show a list of all the chats the bot knows about. Pick the chat you want to send messages to. Repeat for adding multiple chats.

Note: After setting up a group chat in nymea, you can turn on group privacy for the bot again. Except during setup, it is not necessary for the bot to be able to read group messages.
