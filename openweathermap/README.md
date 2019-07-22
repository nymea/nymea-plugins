# OpenWeatherMap

This plugin allows to get the current weather data from [OpenWeatherMap](http://www.openweathermap.org).
The weather data will be refreshed every 15 minutes automatically, but can also refreshed manually.
The plugin offers two different search methods for the location: if the user searches for a empty string,
the plugin makes an autodetection with the WAN ip and offers the user the found weather stations.
The autodetection function uses the geolocation of your WAN ip and searches all available weather
stations in a radius of 1.5 km. Otherwise the plugin returns the list of the found search results
from the search string.

> Note: If you are using a VPN connection, the autodetection will show the results around your VPN location.
