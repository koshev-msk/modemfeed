# Openwrt Telegram Bot

Telegram bot for router with firmware Lede/Openwrt.

## Implemented functions (Commands via Telegram message)

  - */memory* return RAM info
  - */clients* connected clients
  - */wll_list* wifi clients
  - */wifi_list* wifi info
  - */reboot* reboot the device
  - */wol <mac_address>* wake on lan over the Internet
  - */wanip* WAN ip address
  - */<script_name>* any scripts in the `plugins` directory

## Prerequisites

Telegram bot run under Lede or Openwrt firmware than the first prerequisite is to have Lede/Openwrt installed.

Second prerequisite is to have `curl` package installed. You can do this with command `opkg update && opkg install curl`.

## Installation Steps

### Step one:

- Get your chat_id of Telegram. If you don't know what is your chat_id you can use bot @get_id_bot.

- Get a bot token and start your bot. If you don't know how get it you can use bot @BotFather. Send him /newbot command , name of your new bot and a username. Get the returned string "Use this token to access the HTTP API:" 

### Step two:

Copy the files of this repo under `/` directory of your Lede/Openwrt system.

Set files as executable with commands:

```sh
chmod +x -R /usr/lib/telegram-bot/* /usr/lib/telegram-bot/functions/*
chmod +x /etc/init.d/telegram_bot
service telegram_bot enable
```

### Step three:

Set your variables (bot token and chat id) in `telegram_bot` file under `/etc/config/` dir.

```sh
uci set telegram_bot.config.bot_token='[PUT YOUR BOT TOKEN HERE]'
uci set telegram_bot.config.chat_id='[PUT YOUR CHAT ID HERE]'

uci commit telegram_bot
```

Start `telegram_bot` service with commands:

```sh
service telegram_bot restart
```

Enjoy your bot!
