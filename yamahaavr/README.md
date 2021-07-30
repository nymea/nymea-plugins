# Yamaha AV Receiver

This plugin allows to control your Yamaha (non-MusicCast) AV receivers.

Each supported receiver on your local area network should appear automatically in the system.

Browsing is supported, but can be slow due to the nature of the Yamaha API.
As a nice extra, a random album on a random server can be started with a simple action. 

## Supported Things

* Yamaha RX-V675 (tested)
* Other non-MusicCast Yamaha RX-V devices should also work, but haven't been tested
* Newer Yamaha MusicCast devices aren't supported, as they use a different API

## Manual

* Volumes (and some other variables) are represented by integer in the Yamaha API, but shown as double = int/10 in Yamaha UI, so e.g. API will show -455, but receiver will show -45.5 dB. As the Nymea media interface currently needs volume to be an integer, the plugin will show volume as integer, so e.g. -455 instead of -45.5 dB
* Browsing shortcuts (for browsing a SERVER source) can be added via the Thing settings, to avoid having to go through e.g. ServerName/Music/By Folder/FolderName each time. By adding a "shortcut tree" such as ServerName/Music/By Folder/FolderName to the settings, browsing will start in folder FolderName instead of first showing a list of servers including ServerName; another possibility is e.g. ServerName2/Music/By Album

## Requirements

* nymea and the Yamaha device must be in the same local area network.
* The package "nymea-plugin-yamaha" must be installed.

## More

 [Yamaha Electronics](https://www.yamaha.com/en/) 
