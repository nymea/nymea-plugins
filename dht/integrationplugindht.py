import nymea
import Adafruit_DHT

pollTimer = None

def setupThing(info):
    logger.log("SetupThing for DHT11/22:", info.thing.name)

    try:
        refreshSensor(info.thing)
        logger.log("Refreshed sensor values successfully. Setup went well.")
        info.finish(nymea.ThingErrorNoError)
    except:
        logger.log("Error fetching sensor values. Setup failed.")
        info.finish(nymea.ThingErrorHardwareFailure, "Error reading sensor values. Please verify if the sensor is properly connected and you've selected the correct GPIO.")
        return

    global pollTimer
    if pollTimer is None:
        pollTimer = nymea.PluginTimer(5, pollSensors)


def thingRemoved(thing):
    if len(myThings()) is 0:
        global pollTimer
        del pollTimer
        pollTimer = None


def refreshSensor(thing):
    logger.log("Refreshing sensor values:", thing.name)

    if thing.thingClassId == dht11ThingClassId:
        gpio = thing.paramValue(dht11ThingRpiGpioParamTypeId)
        logger.log("GPIO pin:", gpio)
        humidity, temperature = Adafruit_DHT.read_retry(Adafruit_DHT.DHT11, gpio)
        logger.log("Temperature: %s, Humidity: %s" % (temperature, humidity))

        thing.setStateValue(dht11TemperatureStateTypeId, temperature)
        thing.setStateValue(dht11HumidityStateTypeId, humidity)

    elif thing.thingClassId == dht22ThingClassId:
        gpio = thing.paramValue(dht22ThingRpiGpioParamTypeId)
        logger.log("GPIO pin:", gpio)
        humidity, temperature = Adafruit_DHT.read_retry(Adafruit_DHT.DHT22, gpio)
        logger.log("Temperature: %s, Humidity: %s" % (temperature, humidity))

        thing.setStateValue(dht22TemperatureStateTypeId, temperature)
        thing.setStateValue(dht22HumidityStateTypeId, humidity)

    else:
        logger.crit("Unhandled thing class:", thing.thingClassId)


def pollSensors():
    for thing in myThings():
        try:
            refreshSensor(thing)
        except:
            logger.warn("Error refreshing sensor values for", thing.name)


