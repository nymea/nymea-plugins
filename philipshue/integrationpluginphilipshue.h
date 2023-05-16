/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINPHILIPSHUE_H
#define INTEGRATIONPLUGINPHILIPSHUE_H

#include "integrations/integrationplugin.h"
#include "huebridge.h"
#include "huelight.h"
#include "hueremote.h"
#include "huemotionsensor.h"
#include "huetapdial.h"

#include "plugintimer.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"
#include "network/zeroconf/zeroconfservicebrowser.h"
#include "network/zeroconf/zeroconfserviceentry.h"


class QNetworkReply;

class IntegrationPluginPhilipsHue: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginphilipshue.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginPhilipsHue();
    ~IntegrationPluginPhilipsHue();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

    void browseThing(BrowseResult *result) override;
    void browserItem(BrowserItemResult *result) override;
    void executeBrowserItem(BrowserActionInfo *info) override;

private slots:
    void lightStateChanged();
    void remoteStateChanged();
    void onRemoteButtonEvent(int buttonCode);

    // Tap Dial
    void onTapDialReachableChanged(bool reachable);
    void onTapDialBatteryLevelChanged(int batteryLevel);
    void onTapDialRotaryEvent(int rotationCode);
    void onTapDialButtonEvent(int buttonCode);


    // Motion sensor
    void onMotionSensorReachableChanged(bool reachable);
    void onMotionSensorBatteryLevelChanged(int batteryLevel);
    void onMotionSensorTemperatureChanged(double temperature);
    void onMotionSensorPresenceChanged(bool presence);
    void onMotionSensorLightIntensityChanged(double lightIntensity);

private slots:
    void networkManagerReplyReady();
    void onDeviceNameChanged();

private:
    QString normalizeBridgeId(const QString &bridgeId);

private:
    class DiscoveryJob {
    public:
        UpnpDiscoveryReply* upnpReply;
        bool upnpDone = false;
        QNetworkReply* nUpnpReply;
        bool nUpnpDone = false;
        ThingDescriptors results;
    };
    ZeroConfServiceBrowser *m_zeroConfBrowser = nullptr;

    void startUpnPDiscovery(ThingDiscoveryInfo *info, DiscoveryJob *discovery);
    void startNUpnpDiscovery(ThingDiscoveryInfo *info, DiscoveryJob *discovery);
    void finishDiscovery(ThingDiscoveryInfo *info, DiscoveryJob* job);

    PluginTimer *m_pluginTimer1Sec = nullptr;
    PluginTimer *m_pluginTimer5Sec = nullptr;
    PluginTimer *m_pluginTimer15Sec = nullptr;

    QList<HueLight *> m_unconfiguredLights;

    QHash<QNetworkReply *, Thing *> m_lightRefreshRequests;
    QHash<QNetworkReply *, Thing *> m_setNameRequests;
    QHash<QNetworkReply *, Thing *> m_bridgeRefreshRequests;
    QHash<QNetworkReply *, Thing *> m_lightsRefreshRequests;
    QHash<QNetworkReply *, Thing *> m_sensorsRefreshRequests;
    QHash<QNetworkReply *, Thing *> m_bridgeLightsDiscoveryRequests;
    QHash<QNetworkReply *, Thing *> m_bridgeSensorsDiscoveryRequests;
    QHash<QNetworkReply *, Thing *> m_bridgeSearchDevicesRequests;

    QHash<HueBridge *, Thing *> m_bridges;
    QHash<HueLight *, Thing *> m_lights;
    QHash<HueRemote *, Thing *> m_remotes;
    QHash<HueTapDial *, Thing *> m_tapDials;
    QHash<HueMotionSensor *, Thing *> m_motionSensors;

    void refreshLight(Thing *thing);
    void refreshBridge(Thing *thing);

    void refreshLights(HueBridge *bridge);
    void refreshSensors(HueBridge *bridge);

    void discoverBridgeDevices(HueBridge *bridge);
    void searchNewDevices(HueBridge *bridge, const QString &serialNumber);

    void setLightName(Thing *thing);
    void setRemoteName(Thing *thing);

    void processBridgeLightDiscoveryResponse(Thing *thing, const QByteArray &data);
    void processBridgeSensorDiscoveryResponse(Thing *thing, const QByteArray &data);
    void processLightRefreshResponse(Thing *thing, const QByteArray &data);
    void processBridgeRefreshResponse(Thing *thing, const QByteArray &data);
    void processLightsRefreshResponse(Thing *thing, const QByteArray &data);
    void processSensorsRefreshResponse(Thing *thing, const QByteArray &data);
    void processSetNameResponse(Thing *thing, const QByteArray &data);

    void bridgeReachableChanged(Thing *thing, bool reachable);

    Thing* bridgeForBridgeId(const QString &id);
    bool lightAlreadyAdded(const QString &uuid);
    bool sensorAlreadyAdded(const QString &uuid);

    int brightnessToPercentage(int brightness);
    int percentageToBrightness(int percentage);

    void abortRequests(QHash<QNetworkReply *, Thing *> requestList, Thing* thing);
};

#endif // INTEGRATIONPLUGINPHILIPSHUE_H