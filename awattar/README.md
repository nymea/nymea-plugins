# aWATTar

This integration allows to receive the current energy market price from [aWATTar GmbH](https://www.awattar.com/). aWattar is currently available in Austria and Germany.

## Supported Things

In following chart you can see an example of the market prices from -12 hours to + 12 hours from the current
time (0).The green line describes the current market price, the red point line describes the average
price of this interval and the red line describes the deviation. If the deviation is positive, the current
price is above the average, if the deviation is negative, the current price is below the average.

In the following chart you can see an example of the market prices from -12 hours to + 12 hours.The green line displays the current market prices, the red-dotted line is the average price of the regarding interval and the red line describes the deviation. If the deviation is positive, the current price is above the average, if the deviation is negative, the current price is below the average.

* -100 % current price equals lowest price in the interval [-12h `<` now `<` + 12h]
* 0 %    current price equals average price in the interval  [-12h `<` now `<` + 12h]
* +100 % current price equals highest price in the interval [-12h `<` now `<` + 12h]

![aWATTar graph](https://raw.githubusercontent.com/guh/nymea-plugins/master/awattar/docs/images/awattar-graph.png "aWATTar graph")
 
## Requirements

* aWattar "Hourly" energy tarif.
* Internet access. 

## More

* [aWATTar Austria](https://www.awattar.com)
* [aWATTar Germany](https://www.awattar.de)
