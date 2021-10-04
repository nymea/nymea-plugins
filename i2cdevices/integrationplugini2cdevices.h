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

#ifndef INTEGRATIONPLUGINPII2CDEVICES_H
#define INTEGRATIONPLUGINPII2CDEVICES_H

#include <integrations/integrationplugin.h>

class I2CDevice;

class IntegrationPluginI2CDevices: public IntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugini2cdevices.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    IntegrationPluginI2CDevices();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;

    void executeAction(ThingActionInfo *info) override;

    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;

private:
    QHash<I2CDevice*, Thing*> m_i2cDevices;

    QHash<int, StateTypeId> m_ads1115ChannelMap;
    QHash<int, StateTypeId> m_ads1115OvervoltageMap;

    QHash<int, StateTypeId> m_pi16adcChannelMap;
    QHash<int, StateTypeId> m_pi16adcOvervoltageMap;
};

#endif
