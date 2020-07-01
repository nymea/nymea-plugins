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

#include "integrationpluginnanoleaf.h"
#include "plugininfo.h"

#include "network/zeroconf/zeroconfservicebrowser.h"
#include "platform/platformzeroconfcontroller.h"

#include <QHash>
#include <QDebug>

IntegrationPluginNanoleaf::IntegrationPluginNanoleaf()
{

}

void IntegrationPluginNanoleaf::init()
{
    m_zeroconfBrowser = hardwareManager()->zeroConfController()->createServiceBrowser("_nanoleafapi._tcp");
}

void IntegrationPluginNanoleaf::discoverThings(ThingDiscoveryInfo *info)
{
    QStringList serialNumbers;
    foreach (const ZeroConfServiceEntry &entry, m_zeroconfBrowser->serviceEntries()) {

        QHostAddress address = QHostAddress(entry.hostAddress().toString());
        ThingDescriptor descriptor(lightPanelsThingClassId, entry.name(), address.toString());
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

        Thing *existingThing = myThings().findByParams(ParamList() << Param(lightPanelsThingSerialNoParamTypeId, serialNo));
        if (existingThing) {
            //For thing rediscovery
            descriptor.setThingId(existingThing->id());
        }

        serialNumbers.append(serialNo);
        qCDebug(dcNanoleaf()) << "Have device" << entry.name() << serialNo << model << firmwareVersion;
        params << Param(lightPanelsThingModelParamTypeId, model);
        params << Param(lightPanelsThingSerialNoParamTypeId, serialNo);
        params << Param(lightPanelsThingFirmwareVersionParamTypeId, firmwareVersion);
        descriptor.setParams(params);

        info->addThingDescriptor(descriptor);
    }
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginNanoleaf::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, tr("On the Nanoleaf controller, hold the on-off button for 5-7 seconds until the LED starts flashing."));
}

void IntegrationPluginNanoleaf::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username)
    Q_UNUSED(secret)
    QHostAddress address = getHostAddress(info->params().paramValue(lightPanelsThingSerialNoParamTypeId).toString());
    uint port = getPort(info->params().paramValue(lightPanelsThingSerialNoParamTypeId).toString());
    Nanoleaf *nanoleaf = createNanoleafConnection(address, port);
    nanoleaf->addUser(); //push button pairing
    m_unfinishedNanoleafConnections.insert(info->thingId(), nanoleaf);
    m_unfinishedPairing.insert(nanoleaf, info);
    connect(info, &ThingPairingInfo::aborted, this, [info, this] {
        Nanoleaf *nanoleaf = m_unfinishedNanoleafConnections.take(info->thingId());
        m_unfinishedPairing.remove(nanoleaf);
        nanoleaf->deleteLater();
    });
}

void IntegrationPluginNanoleaf::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();
    if(thing->thingClassId() == lightPanelsThingClassId) {

        QString thingSerialNo = thing->paramValue(lightPanelsThingSerialNoParamTypeId).toString();
        qCDebug(dcNanoleaf()) << "Setting up Nanoleaf light panel with serial number:" << thingSerialNo;

        pluginStorage()->beginGroup(thing->id().toString());
        QString token = pluginStorage()->value("authToken").toString();
        pluginStorage()->endGroup();

        Nanoleaf *nanoleaf;
        if (m_unfinishedNanoleafConnections.contains(thing->id())) {
            // This setupDevice is called after a discovery
            nanoleaf = m_unfinishedNanoleafConnections.take(thing->id());
            m_nanoleafConnections.insert(thing->id(), nanoleaf);
            return info->finish(Thing::ThingErrorNoError);
        } else {
            // This setupDevice is called after a (re)start, with an already added thing
            QHostAddress address = getHostAddress(thing->paramValue(lightPanelsThingSerialNoParamTypeId).toString());
            int port = getPort(thing->paramValue(lightPanelsThingSerialNoParamTypeId).toString());
            nanoleaf = createNanoleafConnection(address, port);
            nanoleaf->setAuthToken(token);
            nanoleaf->getControllerInfo(); //This is just to check if the thing is available

            m_nanoleafConnections.insert(thing->id(), nanoleaf);
            m_asyncDeviceSetup.insert(nanoleaf, info);
            connect(info, &ThingSetupInfo::aborted, this, [nanoleaf, this](){m_asyncDeviceSetup.remove(nanoleaf);});
            return;
        }
    }
}

void IntegrationPluginNanoleaf::postSetupThing(Thing *thing)
{
    if (thing->thingClassId() == lightPanelsThingClassId) {
        Nanoleaf *nanoleaf = m_nanoleafConnections.value(thing->id());
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


void IntegrationPluginNanoleaf::thingRemoved(Thing *thing)
{
    if(thing->thingClassId() == lightPanelsThingClassId) {
        Nanoleaf *nanoleaf = m_nanoleafConnections.take(thing->id());
        nanoleaf->deleteLater();
    }

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
        m_pluginTimer = nullptr;
    }
}


void IntegrationPluginNanoleaf::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == lightPanelsThingClassId) {
        Nanoleaf *nanoleaf = m_nanoleafConnections.value(thing->id());
        if (!nanoleaf) {
            return info->finish(Thing::ThingErrorHardwareFailure);
        }

        if (action.actionTypeId() == lightPanelsPowerActionTypeId) {
            bool power = action.param(lightPanelsPowerActionPowerParamTypeId).value().toBool();
            QUuid requestId = nanoleaf->setPower(power);
            connect(info, &ThingActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);

        } else if (action.actionTypeId() == lightPanelsBrightnessActionTypeId) {
            int brightness = action.param(lightPanelsBrightnessActionBrightnessParamTypeId).value().toInt();
            QUuid requestId = nanoleaf->setBrightness(brightness);
            connect(info, &ThingActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);

        } else if (action.actionTypeId() == lightPanelsColorActionTypeId) {
            QColor color(action.param(lightPanelsColorActionColorParamTypeId).value().toString());
            QUuid requestId = nanoleaf->setColor(color);
            connect(info, &ThingActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);

        } else if (action.actionTypeId() == lightPanelsColorTemperatureActionTypeId) {
            int colorTemperature = action.param(lightPanelsColorTemperatureActionColorTemperatureParamTypeId).value().toInt();
            QUuid requestId = nanoleaf->setMired(colorTemperature);
            connect(info, &ThingActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        } else if (action.actionTypeId() == lightPanelsAlertActionTypeId) {
            QUuid requestId = nanoleaf->identify();
            connect(info, &ThingActionInfo::aborted,[requestId, this](){m_asyncActions.remove(requestId);});
            m_asyncActions.insert(requestId, info);
        }
    }
}

void IntegrationPluginNanoleaf::browseThing(BrowseResult *result)
{
    Thing *thing = result->thing();
    Nanoleaf *nanoleaf = m_nanoleafConnections.value(thing->id());
    nanoleaf->getEffects();
    m_asyncBrowseResults.insert(nanoleaf, result);
    connect(result, &BrowseResult::aborted, this, [nanoleaf, this]{m_asyncBrowseResults.remove(nanoleaf);});
}

void IntegrationPluginNanoleaf::browserItem(BrowserItemResult *result)
{
    Q_UNUSED(result)
    qCDebug(dcNanoleaf()) << "BrowserItem called";
}

void IntegrationPluginNanoleaf::executeBrowserItem(BrowserActionInfo *info)
{
    Thing *thing = info->thing();
    Nanoleaf *nanoleaf = m_nanoleafConnections.value(thing->id());
    QUuid requestId = nanoleaf->setEffect(info->browserAction().itemId());
    m_asyncBrowserItem.insert(requestId, info);
    connect(info, &BrowserActionInfo::aborted, this, [requestId, this]{m_asyncBrowserItem.remove(requestId);});
}

Nanoleaf *IntegrationPluginNanoleaf::createNanoleafConnection(const QHostAddress &address, int port)
{
    Nanoleaf *nanoleaf = new Nanoleaf(hardwareManager()->networkManager(), address, port, this);
    connect(nanoleaf, &Nanoleaf::authTokenRecieved, this, &IntegrationPluginNanoleaf::onAuthTokenReceived);
    connect(nanoleaf, &Nanoleaf::authenticationStatusChanged, this, &IntegrationPluginNanoleaf::onAuthenticationStatusChanged);
    connect(nanoleaf, &Nanoleaf::requestExecuted, this, &IntegrationPluginNanoleaf::onRequestExecuted);
    connect(nanoleaf, &Nanoleaf::connectionChanged, this, &IntegrationPluginNanoleaf::onConnectionChanged);

    connect(nanoleaf, &Nanoleaf::controllerInfoReceived, this, &IntegrationPluginNanoleaf::onControllerInfoReceived);
    connect(nanoleaf, &Nanoleaf::brightnessReceived, this, &IntegrationPluginNanoleaf::onBrightnessReceived);
    connect(nanoleaf, &Nanoleaf::powerReceived, this, &IntegrationPluginNanoleaf::onPowerReceived);
    connect(nanoleaf, &Nanoleaf::colorModeReceived, this, &IntegrationPluginNanoleaf::onColorModeReceived);
    connect(nanoleaf, &Nanoleaf::saturationReceived, this, &IntegrationPluginNanoleaf::onSaturationReceived);
    connect(nanoleaf, &Nanoleaf::hueReceived, this, &IntegrationPluginNanoleaf::onHueReceived);
    connect(nanoleaf, &Nanoleaf::colorTemperatureReceived, this, &IntegrationPluginNanoleaf::onColorTemperatureReceived);
    connect(nanoleaf, &Nanoleaf::effectListReceived, this, &IntegrationPluginNanoleaf::onEffectListReceived);
    connect(nanoleaf, &Nanoleaf::selectedEffectReceived, this, &IntegrationPluginNanoleaf::onSelectedEffectReceived);
    return nanoleaf;
}

QHostAddress IntegrationPluginNanoleaf::getHostAddress(const QString &serialNumber)
{
    ZeroConfServiceEntry entry;
    foreach (const ZeroConfServiceEntry &e, m_zeroconfBrowser->serviceEntries()) {
        QString entrySerialNo;
        foreach (QString value, entry.txt()) {
            if (value.contains("id=")) {
                entrySerialNo = value.split("=").last();
                break;
            }
        }
        if (serialNumber == entrySerialNo) {
            entry = e;
            break;
        }
    }
    return QHostAddress(entry.hostAddress());
}

uint IntegrationPluginNanoleaf::getPort(const QString &serialNumber)
{
    ZeroConfServiceEntry entry;
    foreach (const ZeroConfServiceEntry &e, m_zeroconfBrowser->serviceEntries()) {
        QString entrySerialNo;
        foreach (QString value, entry.txt()) {
            if (value.contains("id=")) {
                entrySerialNo = value.split("=").last();
                break;
            }
        }
        if (serialNumber == entrySerialNo) {
            entry = e;
            break;
        }
    }
    return entry.port();
}

void IntegrationPluginNanoleaf::onAuthTokenReceived(const QString &token)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    if (m_unfinishedPairing.contains(nanoleaf)) {
        ThingPairingInfo *info = m_unfinishedPairing.take(nanoleaf);
        pluginStorage()->beginGroup(info->thingId().toString());
        pluginStorage()->setValue("authToken", token);
        pluginStorage()->endGroup();
        info->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginNanoleaf::onAuthenticationStatusChanged(bool authenticated)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    if (m_asyncDeviceSetup.contains(nanoleaf)) {
        ThingSetupInfo *info = m_asyncDeviceSetup.take(nanoleaf);
        if (authenticated) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorSetupFailed);
        }
    }
}

void IntegrationPluginNanoleaf::onRequestExecuted(QUuid requestId, bool success)
{
    if (m_asyncActions.contains(requestId)) {
        ThingActionInfo *info = m_asyncActions.take(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    }

    if (m_asyncBrowserItem.contains(requestId)) {
        BrowserActionInfo *info = m_asyncBrowserItem.take(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    }
}

void IntegrationPluginNanoleaf::onConnectionChanged(bool connected)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
        return;
    thing->setStateValue(lightPanelsConnectedStateTypeId, connected);
}

void IntegrationPluginNanoleaf::onControllerInfoReceived(const Nanoleaf::ControllerInfo &controllerInfo)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
        return;
    //qCDebug(dcNanoleaf()) << "Controller Info received" << controllerInfo.name << controllerInfo.firmwareVersion;
    thing->setParamValue(lightPanelsThingFirmwareVersionParamTypeId, controllerInfo.firmwareVersion);
}

void IntegrationPluginNanoleaf::onPowerReceived(bool power)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
        return;
    //qCDebug(dcNanoleaf()) << "Power received" << power;
    thing->setStateValue(lightPanelsPowerStateTypeId, power);
}

void IntegrationPluginNanoleaf::onBrightnessReceived(int percentage)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
        return;
    //qCDebug(dcNanoleaf()) << "Brightness received" << percentage;
    thing->setStateValue(lightPanelsBrightnessStateTypeId, percentage);
}

void IntegrationPluginNanoleaf::onColorReceived(QColor color)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
        return;
    //qCDebug(dcNanoleaf()) << "Color received" << color.toRgb();
    thing->setStateValue(lightPanelsColorStateTypeId, color);
}

void IntegrationPluginNanoleaf::onColorModeReceived(Nanoleaf::ColorMode colorMode)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
        return;
    switch (colorMode) {
    case Nanoleaf::ColorMode::ColorTemperatureMode:
        thing->setStateValue(lightPanelsColorModeStateTypeId, tr("Color temperature"));
        break;
    case Nanoleaf::ColorMode::HueSaturationMode:
        thing->setStateValue(lightPanelsColorModeStateTypeId, tr("Hue/Saturation"));
        break;
    case Nanoleaf::ColorMode::EffectMode:
        thing->setStateValue(lightPanelsColorModeStateTypeId, tr("Effect"));
        break;
    }
}

void IntegrationPluginNanoleaf::onHueReceived(int hue)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
        return;
    //qCDebug(dcNanoleaf()) << "Hue received" << hue;
    QColor color = QColor(thing->stateValue(lightPanelsColorStateTypeId).toString());
    color.setHsv(hue, color.saturation(), color.value());
    thing->setStateValue(lightPanelsColorStateTypeId, color);
}

void IntegrationPluginNanoleaf::onSaturationReceived(int saturation)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
        return;
    //qCDebug(dcNanoleaf()) << "Saturation received" << saturation;
    QColor color = QColor(thing->stateValue(lightPanelsColorStateTypeId).toString());
    color.setHsv(color.hue(), saturation, color.value());
    thing->setStateValue(lightPanelsColorStateTypeId, color);
}

void IntegrationPluginNanoleaf::onEffectListReceived(const QStringList &effects)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
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
        result->finish(Thing::ThingErrorNoError);
    }
}

void IntegrationPluginNanoleaf::onColorTemperatureReceived(int kelvin)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
        return;
    qCDebug(dcNanoleaf()) << "Color temperature received, Kelvin:" << kelvin << "Mired:" << (653-(kelvin/13));
    //NOTE: this is just a rough estimation of the mired value
    //Mired: 153 - 500
    //Kelvin: 1200-6500
    int mired = static_cast<int>(653-(kelvin/13));
    thing->setStateValue(lightPanelsColorTemperatureStateTypeId, mired);
}

void IntegrationPluginNanoleaf::onSelectedEffectReceived(const QString &effect)
{
    Nanoleaf *nanoleaf = static_cast<Nanoleaf *>(sender());
    Thing *thing = myThings().findById(m_nanoleafConnections.key(nanoleaf));
    if (!thing)
        return;
    //qCDebug(dcNanoleaf()) << "Selected effect received" << effect;
    thing->setStateValue(lightPanelsEffectNameStateTypeId, QString(effect).remove('"').remove('*'));
}

