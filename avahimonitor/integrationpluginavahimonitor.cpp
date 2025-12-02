// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "integrationpluginavahimonitor.h"

#include <platform/platformzeroconfcontroller.h>
#include <integrations/thing.h>

#include "plugininfo.h"

#include <QDebug>
#include <QStringList>
#include <QNetworkInterface>
#include <QDateTime>
#include <QTimer>

IntegrationPluginAvahiMonitor::IntegrationPluginAvahiMonitor()
{

}

void IntegrationPluginAvahiMonitor::setupThing(ThingSetupInfo *info)
{
    qCDebug(dcAvahiMonitor()) << "Setup" << info->thing()->name() << info->thing()->params();

    if (!m_serviceBrowser) {
        m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();
        connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, &IntegrationPluginAvahiMonitor::onServiceEntryAdded);
        connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryRemoved, this, &IntegrationPluginAvahiMonitor::onServiceEntryRemoved);
    }

    foreach (const ZeroConfServiceEntry &entry, m_serviceBrowser->serviceEntries()) {
        if (info->thing()->paramValue(avahiThingServiceParamTypeId).toString() == entry.name() &&
                info->thing()->paramValue(avahiThingHostNameParamTypeId).toString() == entry.hostName()) {
            info->thing()->setStateValue(avahiIsPresentStateTypeId, true);
            info->thing()->setStateValue(avahiLastSeenTimeStateTypeId, QDateTime::currentDateTime().toString());
            info->thing()->setStateValue(avahiAddressStateTypeId, entry.hostAddress().toString());
            info->thing()->setStateValue(avahiPortStateTypeId, entry.port());
        }
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginAvahiMonitor::discoverThings(ThingDiscoveryInfo *info)
{
    if (info->thingClassId() != avahiThingClassId) {
        info->finish(Thing::ThingErrorThingClassNotFound);
        return;
    }

    if (!m_serviceBrowser) {
        m_serviceBrowser = hardwareManager()->zeroConfController()->createServiceBrowser();
        connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryAdded, this, &IntegrationPluginAvahiMonitor::onServiceEntryAdded);
        connect(m_serviceBrowser, &ZeroConfServiceBrowser::serviceEntryRemoved, this, &IntegrationPluginAvahiMonitor::onServiceEntryRemoved);
    }

    // give it a bit of time to find things
    QTimer::singleShot(2000, info, [this, info](){
        QList<ThingDescriptor> deviceDescriptors;
        foreach (const ZeroConfServiceEntry &service, m_serviceBrowser->serviceEntries()) {
            ThingDescriptor thingDescriptor(avahiThingClassId, service.name(), service.serviceType() + " (" + service.hostAddress().toString() + ")");
            ParamList params;
            params.append(Param(avahiThingServiceParamTypeId, service.name()));
            params.append(Param(avahiThingHostNameParamTypeId, service.hostName()));
            thingDescriptor.setParams(params);
            foreach (Thing *existingThing, myThings()) {
                if (existingThing->paramValue(avahiThingServiceParamTypeId).toString() == service.name() && existingThing->paramValue(avahiThingHostNameParamTypeId).toString() == service.hostName()) {
                    thingDescriptor.setThingId(existingThing->id());
                    break;
                }
            }
            deviceDescriptors.append(thingDescriptor);
        }

        info->addThingDescriptors(deviceDescriptors);
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginAvahiMonitor::onServiceEntryAdded(const ZeroConfServiceEntry &serviceEntry)
{
    qCDebug(dcAvahiMonitor()) << "Service entry added:" << serviceEntry;
    foreach (Thing *thing, myThings()) {
        if (thing->paramValue(avahiThingServiceParamTypeId).toString() == serviceEntry.name() &&
                thing->paramValue(avahiThingHostNameParamTypeId).toString() == serviceEntry.hostName()) {
            thing->setStateValue(avahiIsPresentStateTypeId, true);
            thing->setStateValue(avahiLastSeenTimeStateTypeId, QDateTime::currentDateTime().toString());
            thing->setStateValue(avahiAddressStateTypeId, serviceEntry.hostAddress().toString());
            thing->setStateValue(avahiPortStateTypeId, serviceEntry.port());
        }
    }
}

void IntegrationPluginAvahiMonitor::onServiceEntryRemoved(const ZeroConfServiceEntry &serviceEntry)
{
    foreach (Thing *thing, myThings()) {
        if (thing->paramValue(avahiThingServiceParamTypeId).toString() == serviceEntry.name() &&
                thing->paramValue(avahiThingHostNameParamTypeId).toString() == serviceEntry.hostName()) {
            thing->setStateValue(avahiIsPresentStateTypeId, false);
        }
    }
}
