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

#include "integrationpluginlukeroberts.h"
#include "plugininfo.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"

#include <QColor>
#include <QRgb>

IntegrationPluginLukeRoberts::IntegrationPluginLukeRoberts()
{

}

void IntegrationPluginLukeRoberts::init()
{
}


void IntegrationPluginLukeRoberts::discoverThings(ThingDiscoveryInfo *info)
{
    if (!hardwareManager()->bluetoothLowEnergyManager()->available())
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is not available on this system."));

    if (!hardwareManager()->bluetoothLowEnergyManager()->enabled())
        return info->finish(Thing::ThingErrorHardwareNotAvailable, QT_TR_NOOP("Bluetooth is disabled. Please enable Bluetooth and try again."));

    BluetoothDiscoveryReply *reply = hardwareManager()->bluetoothLowEnergyManager()->discoverDevices();
    connect(reply, &BluetoothDiscoveryReply::finished, reply, &BluetoothDiscoveryReply::deleteLater);

    connect(reply, &BluetoothDiscoveryReply::finished, info, [this, info, reply](){

        if (reply->error() != BluetoothDiscoveryReply::BluetoothDiscoveryReplyErrorNoError) {
            qCWarning(dcLukeRoberts()) << "Bluetooth discovery error:" << reply->error();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened during Bluetooth discovery."));
            return;
        }

        foreach (const QBluetoothDeviceInfo &deviceInfo, reply->discoveredDevices()) {
            if (deviceInfo.serviceUuids().contains(customControlServiceUuid)) {
                ThingDescriptor descriptor(modelFThingClassId, "Model F", deviceInfo.name() + " (" + deviceInfo.address().toString() + ")");
                ParamList params;

                foreach (Thing *existingThing, myThings()) {
                    if (existingThing->paramValue(modelFThingMacParamTypeId).toString() == deviceInfo.address().toString()) {
                        descriptor.setThingId(existingThing->id());
                        break;
                    }
                }
                params.append(Param(modelFThingMacParamTypeId, deviceInfo.address().toString()));
                descriptor.setParams(params);
                info->addThingDescriptor(descriptor);
            }
        }
        info->finish(Thing::ThingErrorNoError);
    });
}


void IntegrationPluginLukeRoberts::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcLukeRoberts()) << "Setup device" << thing->name() << thing->params();

    if (thing->thingClassId() == modelFThingClassId) {
        QBluetoothAddress address = QBluetoothAddress(thing->paramValue(modelFThingMacParamTypeId).toString());
        QBluetoothDeviceInfo deviceInfo = QBluetoothDeviceInfo(address, thing->name(), 0);

        BluetoothLowEnergyDevice *bluetoothDevice = hardwareManager()->bluetoothLowEnergyManager()->registerDevice(deviceInfo, QLowEnergyController::RandomAddress);

        LukeRoberts *lamp = new LukeRoberts(bluetoothDevice);
        connect(lamp, &LukeRoberts::connectedChanged, this, &IntegrationPluginLukeRoberts::onConnectedChanged);
        connect(lamp, &LukeRoberts::deviceInformationChanged, this, &IntegrationPluginLukeRoberts::onDeviceInformationChanged);
        connect(lamp, &LukeRoberts::statusCodeReveiced, this, &IntegrationPluginLukeRoberts::onStatusCodeReceived);
        connect(lamp, &LukeRoberts::sceneListReceived, this, &IntegrationPluginLukeRoberts::onSceneListReceived);
        connect(lamp, &LukeRoberts::destroyed, this, [this, lamp] {m_lamps.remove(lamp);});

        m_lamps.insert(lamp, thing);
        connect(lamp, &LukeRoberts::deviceInitializationFinished, info, [this, info, lamp](bool success){
            qCDebug(dcLukeRoberts()) << "Lamp device initialization finished";
            Thing *thing = info->thing();

            if (!thing->setupComplete()) {
                if (success) {
                    info->finish(Thing::ThingErrorNoError);
                } else {
                    m_lamps.take(lamp);

                    hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(lamp->bluetoothDevice());
                    lamp->deleteLater();

                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to lamp."));
                }
            }

        });
        lamp->bluetoothDevice()->connectDevice();
    }
}

void IntegrationPluginLukeRoberts::postSetupThing(Thing *thing)
{
    qCDebug(dcLukeRoberts()) << "Post setup device" << thing->name();

    if (!m_reconnectTimer) {
        qCDebug(dcLukeRoberts()) << "Initializing reconnect timer";
        m_reconnectTimer = hardwareManager()->pluginTimerManager()->registerTimer(10);
        connect(m_reconnectTimer, &PluginTimer::timeout, this, &IntegrationPluginLukeRoberts::onReconnectTimeout);
    }
}


void IntegrationPluginLukeRoberts::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == modelFThingClassId) {
        QPointer<LukeRoberts> lamp = m_lamps.key(thing);
        if (lamp.isNull())
            return info->finish(Thing::ThingErrorHardwareFailure);

        if (!lamp->bluetoothDevice()->connected()) {
            return info->finish(Thing::ThingErrorHardwareNotAvailable);
        }

        if (action.actionTypeId() == modelFPowerActionTypeId) {
            bool power = action.param(modelFPowerActionPowerParamTypeId).value().toBool();
            if (!power) {
                lamp->selectScene(0); //Scene 0 is the off scene
            } else {
                lamp->selectScene(0xff); // Scene 0xff is the default scene
            }

        } else if (action.actionTypeId() == modelFBrightnessActionTypeId) {
            int brightness = action.param(modelFBrightnessActionBrightnessParamTypeId).value().toInt();
            lamp->modifyBrightness(brightness);

        } else if (action.actionTypeId() == modelFColorActionTypeId) {
            QColor rgb = QColor::fromRgb(QRgb(action.param(modelFColorActionColorParamTypeId).value().toInt()));
            lamp->setImmediateLight(0, rgb.saturation(), rgb.hue(), rgb.lightness());

        } else if (action.actionTypeId() == modelFColorTemperatureActionTypeId) {
            int kelvin = action.param(modelFColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
            lamp->modifyColorTemperature(kelvin);
        }
    } else {

    }
}


void IntegrationPluginLukeRoberts::thingRemoved(Thing *thing)
{
    qCDebug(dcLukeRoberts()) << "Delete thing" << thing->name();

    if (thing->thingClassId() == modelFThingClassId) {
        if (!m_lamps.values().contains(thing))
            return;

        LukeRoberts *lamp = m_lamps.key(thing);
        m_lamps.remove(lamp);

        hardwareManager()->bluetoothLowEnergyManager()->unregisterDevice(lamp->bluetoothDevice());
        lamp->deleteLater();
    }

    if (myThings().isEmpty()) {
        qCDebug(dcLukeRoberts()) << "Unregistering reconnect timer";
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_reconnectTimer);
        m_reconnectTimer = nullptr;
    }
}

void IntegrationPluginLukeRoberts::browseThing(BrowseResult *result)
{
    LukeRoberts *lamp = m_lamps.key(result->thing());
    if (!lamp) {
        result->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    qDebug(dcLukeRoberts()) << "Browse thing called";
    m_pendingBrowseResults.insert(lamp, result);
    lamp->getSceneList();
}

void IntegrationPluginLukeRoberts::browserItem(BrowserItemResult *result)
{
    LukeRoberts *lamp = m_lamps.key(result->thing());
    if (!lamp) {
        result->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    qDebug(dcLukeRoberts()) << "Browse item called";
}

void IntegrationPluginLukeRoberts::executeBrowserItem(BrowserActionInfo *info)
{
    LukeRoberts *lamp = m_lamps.key(info->thing());
    if (!lamp) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
    lamp->selectScene(info->browserAction().itemId().toInt());
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginLukeRoberts::executeBrowserItemAction(BrowserItemActionInfo *info)
{
    LukeRoberts *lamp = m_lamps.key(info->thing());
    if (!lamp) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }
}

void IntegrationPluginLukeRoberts::onReconnectTimeout()
{
    foreach (LukeRoberts *lamp, m_lamps.keys()) {
        if (!lamp->bluetoothDevice()->connected()) {
            lamp->bluetoothDevice()->connectDevice();
        }
    }
}

void IntegrationPluginLukeRoberts::onConnectedChanged(bool connected)
{
    LukeRoberts *lamp = static_cast<LukeRoberts *>(sender());
    Thing *thing = m_lamps.value(lamp);
    thing->setStateValue(modelFConnectedStateTypeId, connected);
}

void IntegrationPluginLukeRoberts::onDeviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision)
{
    LukeRoberts *lamp = static_cast<LukeRoberts *>(sender());
    Thing *thing = m_lamps.value(lamp);

    qDebug(dcLukeRoberts()) << thing->name() << "Firmware" << firmwareRevision << "Hardware" << hardwareRevision << "Software" << softwareRevision;
    //thing->setStateValue(modelFFirmwareRevisionStateTypeId, firmwareRevision);
    //thing->setStateValue(modelFardwareRevisionStateTypeId, hardwareRevision);
    //thing->setStateValue(modelFSoftwareRevisionStateTypeId, softwareRevision);
}

void IntegrationPluginLukeRoberts::onSceneListReceived(QList<LukeRoberts::Scene> scenes)
{
    LukeRoberts *lamp = static_cast<LukeRoberts *>(sender());
    if (m_pendingBrowseResults.contains(lamp)) {
        BrowseResult *result = m_pendingBrowseResults.value(lamp);
        foreach (LukeRoberts::Scene scene, scenes) {
            BrowserItem item;
            item.setId(QString::number(scene.id));
            item.setDisplayName(scene.name);
            item.setBrowsable(false);
            item.setExecutable(true);
            item.setIcon(BrowserItem::BrowserIconApplication);
            result->addItem(item);
        }
    }
}

void IntegrationPluginLukeRoberts::onStatusCodeReceived(LukeRoberts::StatusCodes statusCode)
{
    qDebug(dcLukeRoberts()) << "Status code received" << statusCode;
}
