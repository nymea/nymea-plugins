# Air Quality Index

This plug-in gets air quality information from http://aqicn.org.
Through the WAN IP address the next nearby sensor station will be discovered.
The geo location can also be set manually. 

## Supported Things

* Air Quality Index
	* Location discovery
	* Manually location set
	* Air Quality
	* Cautionary statement
	* PM2.5 pollution level
	* PM10 pollution lebel
	* Ozone level
	* Nitrogen dioxide level
	* Carbon monoxide level
	* Sulfur dioxide level
	* Temperature
	* Humidity
	* Pressure
	* Wind speed

NOTE: If you encounter that a value stays to zero, it means the sensor station
doesn't support that value.

Besides the air pollution level the plug-in also states a cautionary statement.
Both states can be used to let nymea notify you about the pollution level and
inform you what precautions should be taken.

## Requirments

* Valid "Air Quality Index" API Key
* The package "nymea-plugin-airqualityindex" must be installed
* Internet connection

## More

More about the different Air Quality Levels: https://www.airnow.gov/index.cfm?action=aqibasics.aqi

