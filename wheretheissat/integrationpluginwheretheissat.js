import { getSatellites } from "./wheretheissat.mjs";
import { getSatelliteData } from "./wheretheissat.mjs";

var pluginTimer;

export function init() {
    console.warn("JS init called!");
}

export function discoverThings(info) {
    getSatellites( (response) => {
        for (var i = 0; i < response.length; i++) {
            var params = [];
            var param = {};
            param.paramTypeId = "1d10b8e2-aea4-495c-8b1f-f44ef088f667";
            param.value = response[i].id
            params.push(param)
            info.addThingDescriptor("4bdedd0b-e268-4671-9b3e-948c853b7b9b", response[i].name, "Satellite", params);
       };
       info.finish(Thing.ThingErrorNoError);
    });
}

export function setupThing(info) {
    var device = info.thing;
    var satelliteId = info.thing.paramValue("1d10b8e2-aea4-495c-8b1f-f44ef088f667");
    if (satelliteId === undefined) {
        console.warn("Could not find satelliteId in params")
        info.finish(Thing.ThingErrorMissingParameter);
        return;
    }

    getSatelliteData(satelliteId, (response) => {
        device.setStateValue("d702143f-9454-412b-88df-83b5655a5000", response.latitude)
        device.setStateValue("42d9b2a1-0e51-4a92-9804-8f6f68ed6fd1", response.longitude)
        info.finish(Thing.ThingErrorNoError);

        pluginTimer = hardwareManager.pluginTimerManager.registerTimer(5);
        pluginTimer.timeout.connect(function() {
            getSatelliteData(satelliteId, (response) => {
                device.setStateValue("d702143f-9454-412b-88df-83b5655a5000", response.latitude)
                device.setStateValue("42d9b2a1-0e51-4a92-9804-8f6f68ed6fd1", response.longitude)
            })
        })
    });
}

export function thingRemoved(thing) {
    hardwareManager.pluginTimerManager.unregisterTimer(pluginTimer);
}

