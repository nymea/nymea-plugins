export function getSatellites(callback) {
    var request = new XMLHttpRequest()
    request.open("GET", "https://api.wheretheiss.at/v1/satellites");
    request.onload = function() {
        var response = JSON.parse(request.response);
        callback(response);
    }
    request.send();
}

export function getSatelliteData(satelliteId, callback) {
    var request = new XMLHttpRequest()
    request.open("GET", "https://api.wheretheiss.at/v1/satellites/" + satelliteId);
    request.onload = function() {
        var data = JSON.parse(request.response);
        callback(data);
    }
    request.send();
}

