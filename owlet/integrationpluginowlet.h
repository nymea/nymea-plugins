/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

#ifndef INTEGRATIONPLUGINOWLET_H
#define INTEGRATIONPLUGINOWLET_H

#include "integrations/integrationplugin.h"
#include "extern-plugininfo.h"

#include "owletserialclient.h"

class ZeroConfServiceBrowser;
class OwletClient;

class IntegrationPluginOwlet: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginowlet.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginOwlet();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void executeAction(ThingActionInfo *info) override;
    void thingRemoved(Thing *thing) override;

private:
    ZeroConfServiceBrowser *m_zeroConfBrowser = nullptr;

    QHash<Thing *, OwletClient *> m_clients;
    QHash<Thing *, OwletSerialClient *> m_serialClients;

    QHash<ThingClassId, ParamTypeId> m_owletIdParamTypeMap;

    // Serial owlets
    QHash<ThingClassId, ParamTypeId> m_owletSerialPortParamTypeMap;
    QHash<ThingClassId, ParamTypeId> m_owletSerialPinParamTypeMap;

    QHash<ParamTypeId, quint8> m_arduinoUnoPinMapping;
    QHash<ParamTypeId, quint8> m_arduinoMiniPro5VPinMapping;
    QHash<ParamTypeId, quint8> m_arduinoMiniPro3VPinMapping;
    QHash<ParamTypeId, quint8> m_arduinoNanoPinMapping;
    OwletSerialClient::PinMode getPinModeFromSettingsValue(const QString &settingsValue);

    void setupArduinoChildThing(OwletSerialClient *client, quint8 pinId, OwletSerialClient::PinMode pinMode);
    void configurePin(OwletSerialClient *client, quint8 pinId, OwletSerialClient::PinMode pinMode);
    QString getPinName(Thing *parent, quint8 pinId);
};

#endif // INTEGRATIONPLUGINOWLET_H
