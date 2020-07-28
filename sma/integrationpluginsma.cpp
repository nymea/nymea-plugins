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

#include "integrationpluginsma.h"
#include "plugininfo.h"

IntegrationPluginSma::IntegrationPluginSma()
{

}

void IntegrationPluginSma::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (!m_udpSocket) {
        m_udpSocket = new QUdpSocket(this);
    }

    if (thing->thingClassId() == sunnyWebBoxThingClassId) {
        //check if a Sunny WebBox is already added with this IPv4Address
        foreach(SunnyWebBox *sunnyWebBox, m_sunnyWebBoxes.values()) {
            if(sunnyWebBox->hostAddress().toString() == thing->paramValue(sunnyWebBoxThingHostParamTypeId).toString()){
                //this logger at this IPv4 address is already added
                qCWarning(dcSma()) << "thing at " << thing->paramValue(sunnyWebBoxThingHostParamTypeId).toString() << " already added!";
                info->finish(Thing::ThingErrorThingInUse);
                return;
            }
        }

    } else if (thing->thingClassId() == inverterThingClassId) {
        Thing *parentThing = myThings().findById(thing->parentId());
        if (!parentThing) {
            qCWarning(dcSma()) << "Could not find parentThing for thing " << thing->name();
            return info->finish(Thing::ThingErrorHardwareNotAvailable, "Please try again");
        }
        if (!parentThing->setupComplete()) {
            //wait for the parent to finish the setup process
            connect(parentThing, &Thing::setupStatusChanged, info, [this, info, parentThing] {

                if (parentThing->setupComplete())
                    setupChild(info, parentThing);
            });
            return;
        }
        setupChild(info, parentThing);
    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginSma::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == sunnyWebBoxThingClassId) {
        SunnyWebBox *sunnyWebBox = m_sunnyWebBoxes.value(thing);
        if (!sunnyWebBox)
            return;
        sunnyWebBox->getDevices();
    } else if (thing->thingClassId() == inverterThingClassId) {
    }
}

void IntegrationPluginSma::thingRemoved(Thing *thing)
{
    if (thing->thingClassId() == sunnyWebBoxThingClassId) {
        m_sunnyWebBoxes.take(thing)->deleteLater();
    }

    if (myThings().filterByThingClassId(sunnyWebBoxThingClassId).isEmpty()) {
        m_udpSocket->deleteLater();
        m_udpSocket = nullptr;
    }
}

SunnyWebBox * IntegrationPluginSma::createSunnyWebBoxConnection(Thing *thing)
{
    SunnyWebBox *sunnyWebBox = new SunnyWebBox(m_udpSocket, this);
    m_sunnyWebBoxes.insert(thing, sunnyWebBox);
    //connect();
    return sunnyWebBox;
}

void IntegrationPluginSma::setupChild(ThingSetupInfo *info, Thing *parentThing)
{
    Q_UNUSED(info)
    Q_UNUSED(parentThing)
}
