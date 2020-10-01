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

#ifndef INTEGRATIONPLUGINYEELIGHT_H
#define INTEGRATIONPLUGINYEELIGHT_H

#include "integrations/integrationplugin.h"
#include "plugintimer.h"
#include "network/networkaccessmanager.h"
#include "network/upnp/upnpdiscovery.h"
#include "yeelight.h"
#include "ssdp.h"

#include <QTimer>

class IntegrationPluginYeelight : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginyeelight.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginYeelight();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

private:
    Ssdp *m_ssdp = nullptr;
    PluginTimer *m_pluginTimer = nullptr;
    QHash<int, ThingActionInfo *> m_asyncActions;
    QHash<ThingId, Yeelight *> m_yeelightConnections;

    QHash<ThingClassId, StateTypeId> m_connectedStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_powerStateTypeIds;
    QHash<ThingClassId, StateTypeId> m_brightnessStateTypeIds;

    QHash<ThingId, ThingActionInfo *> m_pendingBrightnessAction;

private slots:
    void onDeviceNameChanged();
    void onConnectionChanged(bool connected);
    void onRequestExecuted(int requestId, bool success);

    void onPowerNotificationReceived(bool status);
    void onBrightnessNotificationReceived(int percentage);
    void onColorTemperatureNotificationReceived(int kelvin);
    void onRgbNotificationReceived(QRgb rgbColor);
    void onNameNotificationReceived(const QString &name);
    void onColorModeNotificationReceived(Yeelight::YeelightColorMode colorMode);
};

#endif // INTEGRATIONPLUGINYEELIGHT_H
