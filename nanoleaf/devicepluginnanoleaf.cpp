/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by copyright law, and
* remains the property of nymea GmbH. All rights, including reproduction, publication,
* editing and translation, are reserved. The use of this project is subject to the terms of a
* license agreement to be concluded with nymea GmbH in accordance with the terms
* of use of nymea GmbH, available under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* This project may also contain libraries licensed under the open source software license GNU GPL v.3.
* Alternatively, this project may be redistributed and/or modified under the terms of the GNU
* Lesser General Public License as published by the Free Software Foundation; version 3.
* this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along with this project.
* If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under contact@nymea.io
* or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicepluginnanoleaf.h"
#include "plugininfo.h"

#include "network/zeroconf/zeroconfservicebrowser.h"
#include "platform/platformzeroconfcontroller.h"

#include <QHash>
#include <QDebug>

DevicePluginNanoleaf::DevicePluginNanoleaf()
{

}

void DevicePluginNanoleaf::init()
{
    m_zeroconfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_nanoleafapi._tcp");
}

void DevicePluginNanoleaf::discoverDevices(DeviceDiscoveryInfo *info)
{
    QStringList serialNumbers;
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {

        DeviceDescriptor descriptor(lightPanelsDeviceClassId, entry.name(), entry.hostAddress().toString());
        ParamList params;

        QString serialNo;
        QString model;
        QString firmwareVersion;

        foreach (QString value, entry.txt()) {
            if (value.contains("id=")) {
                serialNo = value.split("=").last();
            } else if (value.contains("md=")) {
                model = value.split("=").last();
            } else if (value.contains("srcvers=")) {
                firmwareVersion = value.split("=").last();
            }
        }
        if (serialNumbers.contains(serialNo)) {
            continue; //To avoid duplicated devices
        }

        Device *existingDevice = myDevices().findByParams(ParamList() << Param(lightPanelsDeviceSerialNoParamTypeId, serialNo));
        if (existingDevice) {
            //For device rediscovery
            descriptor.setDeviceId(existingDevice->id());
        }

        serialNumbers.append(serialNo);
        qCDebug(dcNanoleaf()) << "Have device" << entry.name() << serialNo << model << firmwareVersion;
        params << Param(lightPanelsDeviceAddressParamTypeId, entry.hostAddress().toString());
        params << Param(lightPanelsDevicePortParamTypeId, entry.port());
        params << Param(lightPanelsDeviceModelParamTypeId, model);
        params << Param(lightPanelsDeviceSerialNoParamTypeId, serialNo);
        params << Param(lightPanelsDeviceFirmwareVersionParamTypeId, firmwareVersion);
        descriptor.setParams(params);

        info->addDeviceDescriptor(descriptor);
    }
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginNanoleaf::startPairing(DevicePairingInfo *info)
{
    info->finish(Device::DeviceErrorNoError, tr("On the Nanoleaf controller, hold the on-off button for 5-7 seconds until the LED starts flashing."));
}

void DevicePluginNanoleaf::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)
    Q_UNUSED(secret)
    Nanoleaf *nanoleaf = createNanoleafConnection(QHostAddress(info->params().paramValue(lightPanelsDeviceAddressParamTypeId).toString()), info->params().paramValue(lightPanelsDevicePortParamTypeId).toInt());
    nanoleaf->addUser(); //push button pairing
    m_unfinishedNanoleafConnections.insert(info->deviceId(), nanoleaf);
    m_unfinishedPairing.insert(nanoleaf, info);
    connect(info, &DevicePairingInfo::aborted, this, [info, this] {
        Nanoleaf *nanoleaf = m_unfinishedNanoleafConnections.take(info->deviceId());
        m_unfinishedPairing.remove(nanoleaf);
        nanoleaf->deleteLater();
    });
}

void DevicePluginNanoleaf::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();
    if(device->deviceClassId() == lightPanelsDeviceClassId) {
        pluginStorage()->beginGroup(device->id().toString());
        QString token = pluginStorage()->value("authToken").toString();
        pluginStorage()->endGroup();

        Nanoleaf *nanoleaf;
        if (m_unfinishedNanoleafConnections.contains(device->id())) {
            // This setupDevice is called after a discovery
            nanoleaf = m_unfinishedNanoleafConnections.take(device->id());
            m_nanoleafConnections.insert(device->id(), nanoleaf);
            return info->finish(Device::DeviceErrorNoError);
        } else {
            // This setupDevice is called after a (re)start, with an already added device
            QHostAddress address(device->paramValue(lightPanelsDeviceAddressParamTypeId).toString());
            int port = device->paramValue(lightPanelsDevicePortParamTypeId).toInt();
            nanoleaf = createNanoleafConnection(address, port);
            nanoleaf->setAuthToken(token);
            nanoleaf->getControllerInfo(); //This is just to check if the device is available

            m_nanoleafConnections.insert(device->id(), nanoleaf);
            m_asyncDeviceSetup.insert(nanoleaf, info);
            connect(info, &DeviceSetupInfo::aborted, this, [nanoleaf, this](){m_asyncDeviceSetup.remove(nanoleaf);});
            return;
        }
    }
}

void DevicePluginNanoleaf::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == lightPanelsDeviceClassId) {
        Nanoleaf *nanoleaf = m_nanoleafConnections.value(device->id());
        if (!nanoleaf)
            return;
         nanoleaf->getControllerInfo();
         nanoleaf->registerForEvents();
    }

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(5);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {
            foreach (Nanoleaf *nanoleaf, m_nanoleafConnections) {
                nanoleaf->getControllerInfo();
            }
        });
    }
}


void DevicePluginNanoleaf::deviceRemoved(Device *device)
{
    if(device->deviceClassId() == lightPanelsDeviceClassId) {
        Nanoleaf *nanoleaf = m_nanoleafConnections.take(device->id());
        nanoleaf->deleteLater();
    }

    if (myDevices().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void DevicePluginNanoleaf::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == lightPanelsDeviceClassId) {
        Nanoleaf *nanoleaf = m_nanoleafConnections.value(device->id());
        if (!nanoleaf) {
            return info->finish(Device::DeviceErrorHardwareFailure);
        }

        if (action.actionTypeId() == lightPanelsPowerActionTypeId) {
            bool power = action.param(lightPanelsPowerActionPowerParamTypeId).value().toBool();
            QUuid requestId = nanoleaf->setPower(power);
            connect(info, &DeviceActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);

        } else if (action.actionTypeId() == lightPanelsBrightnessActionTypeId) {
            int brightness = action.param(lightPanelsBrightnessActionBrightnessParamTypeId).value().toInt();
            QUuid requestId = nanoleaf->setBrightness(brightness);
            connect(info, &DeviceActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);

        } else if (action.actionTypeId() == lightPanelsColorActionTypeId) {
            QColor color(action.param(lightPanelsColorActionColorParamTypeId).value().toString());
            QUuid requestId = nanoleaf->setColor(color);
            connect(info, &DeviceActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);

        } else if (action.actionTypeId() == lightPanelsColorTemperatureActionTypeId) {
            int colorTemperature = action.param(lightPanelsColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
            QUuid requestId = nanoleaf->setMired(colorTemperature);
            connect(info, &DeviceActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == lightPanelsAlertActionTypeId) {
            QUuid requestId = nanoleaf->identify();
            connect(info, &DeviceActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        }
    }
}

void DevicePluginNanoleaf::browseDevice(BrowseResult *result)
{
    Device *device = result->device();
    Nanoleaf *nanoleaf = m_nanoleafConnections.value(device->id());
    nanoleaf->getEffects();
    m_asyncBrowseResults.insert(nanoleaf, result);
    connect(result, &BrowseResult::aborted, this, [nanoleaf, this]{m_asyncBrowseResults.remove(nanoleaf);});
}

void DevicePluginNanoleaf::browserItem(BrowserItemResult *result)
{
    Q_UNUSED(result)
    qCDebug(dcNanoleaf()) << "BrowserItem called";
}

void DevicePluginNanoleaf::executeBrowserItem(BrowserActionInfo *info)
{
    Device *device = info->device();
    Nanoleaf *nanoleaf = m_nanoleafConnections.value(device->id());
    QUuid requestId = nanoleaf->setEffect(info->browserAction().itemId());
    m_asyncBrowserItem.insert(requestId, info);
    connect(info, &BrowserActionInfo::aborted, this, [requestId, this]{m_asyncBrowserItem.remove(requestId);});
}

Nanoleaf *DevicePluginNanoleaf::createNanoleafConnection(const QHostAddress &address, int port)
{
    Nanoleaf *nanoleaf = new Nanoleaf(hardwareManager()->networkManager(), address, port, this);
    connect(nanoleaf, &Nanoleaf::authTokenRecieved, this, &DevicePluginNanoleaf::onAuthTokenReceived);
    connect(nanoleaf, &Nanoleaf::authenticationStatusChanged, this, &DevicePluginNanoleaf::onAuthenticationStatusChanged);
    connect(nanoleaf, &Nanoleaf::requestExecuted, this, &DevicePluginNanoleaf::onRequestExecuted);
    connect(nanoleaf, &Nanoleaf::connectionChanged, this, &DevicePluginNanoleaf::onConnectionChanged);

    connect(nanoleaf, &Nanoleaf::controllerInfoReceived, this, &DevicePluginNanoleaf::onControllerInfoReceived);
    connect(nanoleaf, &Nanoleaf::brightnessReceived, this, &DevicePluginNanoleaf::onBrightnessReceived);
    connect(nanoleaf, &Nanoleaf::powerReceived, this, &DevicePluginNanoleaf::onPowerReceived);
    connect(nanoleaf, &Nanoleaf::colorModeReceived, this, &DevicePluginNanoleaf::onColorModeReceived);
    connect(nanoleaf, &Nanoleaf::saturationReceived, this, &DevicePluginNanoleaf::onSaturationReceived);
    connect(nanoleaf, &Nanoleaf::hueReceived, this, &DevicePluginNanoleaf::onHueReceived);
    connect(nanoleaf, &Nanoleaf::colorTemperatureReceived, this, &DevicePluginNanoleaf::onColorTemperatureReceived);
    connect(nanoleaf, &Nanoleaf::effectListReceived, this, &DevicePluginNanoleaf::onEffectListReceived);
    connect(nanoleaf, &Nanoleaf::selectedEffectReceived, this, &DevicePluginNanoleaf::onSelectedEffectReceived);
    return nanoleaf;
}

void DevicePluginNanoleaf::onAuthTokenReceived(const QString &token)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    if (m_unfinishedPairing.contains(nanoleaf)) {
        DevicePairingInfo *info = m_unfinishedPairing.take(nanoleaf);
        pluginStorage()->beginGroup(info->deviceId().toString());
        pluginStorage()->setValue("authToken", token);
        pluginStorage()->endGroup();
        info->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginNanoleaf::onAuthenticationStatusChanged(bool authenticated)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    if (m_asyncDeviceSetup.contains(nanoleaf)) {
        DeviceSetupInfo *info = m_asyncDeviceSetup.take(nanoleaf);
        if (authenticated) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorSetupFailed);
        }
    }
}

void DevicePluginNanoleaf::onRequestExecuted(QUuid requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        DeviceActionInfo *info = m_asyncActions.take(requestId);
        if (success) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorHardwareNotAvailable);
        }
    }

    if (m_asyncBrowserItem.contains(requestId)) {
        BrowserActionInfo *info = m_asyncBrowserItem.take(requestId);
        if (success) {
            info->finish(Device::DeviceErrorNoError);
        } else {
            info->finish(Device::DeviceErrorHardwareNotAvailable);
        }
    }
}

void DevicePluginNanoleaf::onConnectionChanged(bool connected)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    device->setStateValue(lightPanelsConnectedStateTypeId, connected);
}

void DevicePluginNanoleaf::onControllerInfoReceived(const Nanoleaf::ControllerInfo &controllerInfo)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    //qCDebug(dcNanoleaf()) << "Controller Info received" << controllerInfo.name << controllerInfo.firmwareVersion;
    device->setParamValue(lightPanelsDeviceFirmwareVersionParamTypeId, controllerInfo.firmwareVersion);
}

void DevicePluginNanoleaf::onPowerReceived(bool power)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    //qCDebug(dcNanoleaf()) << "Power received" << power;
    device->setStateValue(lightPanelsPowerStateTypeId, power);
}

void DevicePluginNanoleaf::onBrightnessReceived(int percentage)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    //qCDebug(dcNanoleaf()) << "Brightness received" << percentage;
    device->setStateValue(lightPanelsBrightnessStateTypeId, percentage);
}

void DevicePluginNanoleaf::onColorReceived(QColor color)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    //qCDebug(dcNanoleaf()) << "Color received" << color.toRgb();
    device->setStateValue(lightPanelsColorStateTypeId, color);
}

void DevicePluginNanoleaf::onColorModeReceived(Nanoleaf::ColorMode colorMode)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    switch (colorMode) {
    case Nanoleaf::ColorMode::ColorTemperatureMode:
        device->setStateValue(lightPanelsColorModeStateTypeId, tr("Color temperature"));
        break;
    case Nanoleaf::ColorMode::HueSaturationMode:
        device->setStateValue(lightPanelsColorModeStateTypeId, tr("Hue/Saturation"));
        break;
    case Nanoleaf::ColorMode::EffectMode:
        device->setStateValue(lightPanelsColorModeStateTypeId, tr("Effect"));
        break;
    }
}

void DevicePluginNanoleaf::onHueReceived(int hue)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    //qCDebug(dcNanoleaf()) << "Hue received" << hue;
    QColor color = QColor(device->stateValue(lightPanelsColorStateTypeId).toString());
    color.setHsv(hue, color.saturation(), color.value());
    device->setStateValue(lightPanelsColorStateTypeId, color);
}

void DevicePluginNanoleaf::onSaturationReceived(int saturation)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    //qCDebug(dcNanoleaf()) << "Saturation received" << saturation;
    QColor color = QColor(device->stateValue(lightPanelsColorStateTypeId).toString());
    color.setHsv(color.hue(), saturation, color.value());
    device->setStateValue(lightPanelsColorStateTypeId, color);
}

void DevicePluginNanoleaf::onEffectListReceived(const QStringList &effects)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    //qCDebug(dcNanoleaf()) << "Effect list received" << effects;

    if (m_asyncBrowseResults.contains(nanoleaf)) {
        BrowseResult *result = m_asyncBrowseResults.take(nanoleaf);
        foreach (QString effect, effects) {
            BrowserItem item;
            item.setId(effect);
            item.setBrowsable(false);
            item.setExecutable(true);
            item.setDisplayName(effect);
            item.setDisabled(false);
            result->addItem(item);
        }
        result->finish(Device::DeviceErrorNoError);
    }
}

void DevicePluginNanoleaf::onColorTemperatureReceived(int kelvin)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    //qCDebug(dcNanoleaf()) << "Color temperature received" << kelvin;
    //NOTE: this is just a rough estimation of the mired value
    int mired = static_cast<int>(kelvin/11.12); //FIXME
    device->setStateValue(lightPanelsColorTemperatureStateTypeId, mired);
}

void DevicePluginNanoleaf::onSelectedEffectReceived(const QString &effect)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Device *device = myDevices().findById(m_nanoleafConnections.key(nanoleaf));
    if (!device)
        return;
    //qCDebug(dcNanoleaf()) << "Selected effect received" << effect;
    device->setStateValue(lightPanelsEffectNameStateTypeId, QString(effect).remove('"').remove('*'));
}

