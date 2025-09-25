/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#ifndef INTEGRATIONPLUGINGPIO_H
#define INTEGRATIONPLUGINGPIO_H

#include <integrations/integrationplugin.h>
#include <plugintimer.h>

#include "gpiodescriptor.h"

// libnymea-gpio
#include <gpio.h>
#include <gpiomonitor.h>
#include <gpiobutton.h>

class IntegrationPluginGpio : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugingpio.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginGpio();

    void init() override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void executeAction(ThingActionInfo *info) override;

private:
    QHash<ThingClassId, ParamTypeId> m_gpioParamTypeIds;
    QHash<ThingClassId, ParamTypeId> m_activeLowParamTypeIds;

    QHash<Gpio *, Thing *> m_gpioDevices;
    QHash<GpioMonitor *, Thing *> m_monitorDevices;
    QHash<GpioButton *, Thing *> m_buttonDevices;

    QHash<int, Gpio *> m_raspberryPiGpios;
    QHash<int, GpioMonitor *> m_raspberryPiGpioMoniors;
    QHash<int, GpioButton *> m_raspberryPiGpioButtons;

    QHash<int, Gpio *> m_beagleboneBlackGpios;
    QHash<int, GpioMonitor *> m_beagleboneBlackGpioMoniors;
    QHash<int, GpioButton *> m_beagleboneBlackGpioButtons;

    QList<GpioDescriptor> raspberryPiGpioDescriptors();
    QList<GpioDescriptor> beagleboneBlackGpioDescriptors();
    PluginTimer *m_counterTimer = nullptr;
    QHash<ThingId, int> m_counterValues;

};

#endif // INTEGRATIONPLUGINGPIO_H
