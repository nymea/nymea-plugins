# aWattar

This integration plug-in allows to receive the current energy market price from [aWATTar GmbH](https://www.awattar.com/).

The Austrian edition of aWATTar requires to enter the access token from your energy provider.
You can find more information about you accesstoken [here](https://www.awattar.com/api-unser-datenfeed).


## Supported Things

In following chart you can see an example of the market prices from -12 hours to + 12 hours from the current
time (0).The green line describes the current market price, the red point line describes the average
price of this interval and the red line describes the deviation. If the deviation is positive, the current
price is above the average, if the deviation is negative, the current price is below the average.

* -100 % current price equals lowest price in the interval [-12h < now < + 12h]
* 0 %    current price equals average price in the interval  [-12h < now < + 12h]
* +100 % current price equals highest price in the interval [-12h < now < + 12h]

![aWATTar graph](https://raw.githubusercontent.com/guh/nymea-plugins/master/awattar/docs/images/awattar-graph.png "aWATTar graph")
 
<<<<<<< HEAD
=======
## Requirements

* Valid aWattar access token for aWATTar Austria.
    * The German servers do not require a token at this point.
* aWattar "Hourly" energy tarif.
* Internet access. 

## More

* [aWATTar Austria](https://www.awattar.com)
* [aWATTar Germany](https://www.awattar.de)
