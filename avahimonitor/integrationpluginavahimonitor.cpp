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

#include "integrationpluginavahimonitor.h"

#include "integrations/thing.h"
#include "plugininfo.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"

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
            info->thing()->setStateValue(avahiLastSeenTimeStateTypeId, QDateTime::currentDateTime());
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
            thing->setStateValue(avahiLastSeenTimeStateTypeId, QDateTime::currentDateTime());
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
