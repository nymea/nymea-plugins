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


#include "integrationpluginbluos.h"
#include "plugininfo.h"
#include "integrations/thing.h"
#include "network/networkaccessmanager.h"
#include "platform/platformzeroconfcontroller.h"
#include "network/zeroconf/zeroconfservicebrowser.h"

#include <QDebug>
#include <QStringList>
#include <QJsonDocument>
#include <QTimer>


IntegrationPluginBluOS::IntegrationPluginBluOS()
{

}

IntegrationPluginBluOS::~IntegrationPluginBluOS()
{

}

void IntegrationPluginBluOS::init()
{

}

void IntegrationPluginBluOS::discoverThings(ThingDiscoveryInfo *info)
{

    if (info->thingClassId() == bluosThingClassId) {
        /*
        * The HEOS products can be discovered using the UPnP SSDP protocol. Through discovery,
        * the IP address of the HEOS products can be retrieved. Once the IP address is retrieved,
        * a telnet connection to port 1255 can be opened to access the HEOS CLI and control the HEOS system.
        * The HEOS product IP address can also be set statically and manually programmed into the control system.
        * Search target name (ST) in M-SEARCH discovery request is 'urn:schemas-denon-com:thing:ACT-Denon:1'.
        */
        UpnpDiscoveryReply *reply = hardwareManager()->upnpDiscovery()->discoverDevices();
        connect(reply, &UpnpDiscoveryReply::finished, info, [this, reply, info](){
            reply->deleteLater();

            if (reply->error() != UpnpDiscoveryReply::UpnpDiscoveryReplyErrorNoError) {
                qCWarning(dcDenon()) << "Upnp discovery error" << reply->error();
                info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("UPnP discovery failed."));
                return;
            }

            foreach (const UpnpDeviceDescriptor &upnpDevice, reply->deviceDescriptors()) {
                qCDebug(dcDenon) << "UPnP thing found:" << upnpDevice.modelDescription() << upnpDevice.friendlyName() << upnpDevice.hostAddress().toString() << upnpDevice.modelName() << upnpDevice.manufacturer() << upnpDevice.serialNumber();

                if (upnpDevice.modelName().contains("HEOS")) {
                    QString serialNumber = upnpDevice.serialNumber();
                    if (serialNumber != "0000001") {
                        // child devices have serial number 0000001
                        qCDebug(dcDenon) << "UPnP thing found:" << upnpDevice.modelDescription() << upnpDevice.friendlyName() << upnpDevice.hostAddress().toString() << upnpDevice.modelName() << upnpDevice.manufacturer() << upnpDevice.serialNumber();
                        ThingDescriptor descriptor(heosThingClassId, upnpDevice.modelName(), serialNumber);
                        ParamList params;
                        foreach (Thing *existingThing, myThings()) {
                            if (existingThing->paramValue(heosThingSerialNumberParamTypeId).toString().contains(serialNumber, Qt::CaseSensitivity::CaseInsensitive)) {
                                descriptor.setThingId(existingThing->id());
                                break;
                            }
                        }
                        params.append(Param(heosThingModelNameParamTypeId, upnpDevice.modelName()));
                        params.append(Param(heosThingIpParamTypeId, upnpDevice.hostAddress().toString()));
                        params.append(Param(heosThingSerialNumberParamTypeId, serialNumber));
                        descriptor.setParams(params);
                        info->addThingDescriptor(descriptor);
                    }
                }
            }
            info->finish(Thing::ThingErrorNoError);
        });
        return;
    }
    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginDenon::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == AVRX1000ThingClassId) {
        qCDebug(dcDenon) << "Setup Denon device" << thing->paramValue(AVRX1000ThingIpParamTypeId).toString();

        QHostAddress address(thing->paramValue(AVRX1000ThingIpParamTypeId).toString());
        if (address.isNull()) {
            qCWarning(dcDenon) << "Could not parse ip address" << thing->paramValue(AVRX1000ThingIpParamTypeId).toString();
            info->finish(Thing::ThingErrorInvalidParameter, QT_TR_NOOP("The given IP address is not valid."));
            return;
        }

        AvrConnection *denonConnection = new AvrConnection(address, 23, this);
        connect(denonConnection, &AvrConnection::connectionStatusChanged, this, &IntegrationPluginDenon::onAvrConnectionChanged);
        connect(denonConnection, &AvrConnection::socketErrorOccured, this, &IntegrationPluginDenon::onAvrSocketError);
        connect(denonConnection, &AvrConnection::channelChanged, this, &IntegrationPluginDenon::onAvrChannelChanged);
        connect(denonConnection, &AvrConnection::powerChanged, this, &IntegrationPluginDenon::onAvrPowerChanged);
        connect(denonConnection, &AvrConnection::volumeChanged, this, &IntegrationPluginDenon::onAvrVolumeChanged);
        connect(denonConnection, &AvrConnection::surroundModeChanged, this, &IntegrationPluginDenon::onAvrSurroundModeChanged);
        connect(denonConnection, &AvrConnection::muteChanged, this, &IntegrationPluginDenon::onAvrMuteChanged);

        m_avrConnections.insert(thing, denonConnection);
        m_asyncAvrSetups.insert(denonConnection, info);
        // In case the setup is cancelled before we finish it...
        connect(info, &QObject::destroyed, this, [this, info, denonConnection]() { m_asyncAvrSetups.remove(denonConnection); });

        denonConnection->connectDevice();
        return;
    }

    if (thing->thingClassId() == heosThingClassId) {
        qCDebug(dcDenon) << "Setup Denon device" << thing->paramValue(heosThingIpParamTypeId).toString();

        QHostAddress address(thing->paramValue(heosThingIpParamTypeId).toString());
        Heos *heos = new Heos(address, this);
        connect(heos, &Heos::connectionStatusChanged, this, &IntegrationPluginDenon::onHeosConnectionChanged);
        connect(heos, &Heos::playerDiscovered, this, &IntegrationPluginDenon::onHeosPlayerDiscovered);
        connect(heos, &Heos::playStateReceived, this, &IntegrationPluginDenon::onHeosPlayStateReceived);
        connect(heos, &Heos::repeatModeReceived, this, &IntegrationPluginDenon::onHeosRepeatModeReceived);
        connect(heos, &Heos::shuffleModeReceived, this, &IntegrationPluginDenon::onHeosShuffleModeReceived);
        connect(heos, &Heos::muteStatusReceived, this, &IntegrationPluginDenon::onHeosMuteStatusReceived);
        connect(heos, &Heos::volumeStatusReceived, this, &IntegrationPluginDenon::onHeosVolumeStatusReceived);
        connect(heos, &Heos::nowPlayingMediaStatusReceived, this, &IntegrationPluginDenon::onHeosNowPlayingMediaStatusReceived);

        m_heos.insert(thing, heos);
        m_asyncHeosSetups.insert(heos, info);
        // In case the setup is cancelled before we finish it...
        connect(info, &QObject::destroyed, this, [this, info, heos]() { m_asyncHeosSetups.remove(heos); });

        heos->connectHeos();
        return;
    }

    if (thing->thingClassId() == heosPlayerThingClassId) {
        info->finish(Thing::ThingErrorNoError);
        return;
    }
    info->finish(Thing::ThingErrorThingClassNotFound);
}
