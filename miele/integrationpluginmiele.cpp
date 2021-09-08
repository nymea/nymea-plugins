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

#include "integrationpluginmiele.h"
#include "plugininfo.h"
#include <cmath>

#include <QJsonDocument>
#include <QUrl>
#include <QUrlQuery>

IntegrationPluginMiele::IntegrationPluginMiele()
{
    m_idParamTypeIds.insert(ovenThingClassId, ovenThingIdParamTypeId);
    m_idParamTypeIds.insert(fridgeThingClassId, fridgeThingIdParamTypeId);
    m_idParamTypeIds.insert(dryerThingClassId, dryerThingIdParamTypeId);
    m_idParamTypeIds.insert(coffeeMakerThingClassId, coffeeMakerThingIdParamTypeId);
    m_idParamTypeIds.insert(dishwasherThingClassId, dishwasherThingIdParamTypeId);
    m_idParamTypeIds.insert(washerThingClassId, washerThingIdParamTypeId);
    m_idParamTypeIds.insert(cookTopThingClassId, cookTopThingIdParamTypeId);
    m_idParamTypeIds.insert(hoodThingClassId, hoodThingIdParamTypeId);
    m_idParamTypeIds.insert(cleaningRobotThingClassId, cleaningRobotThingIdParamTypeId);

    m_connectedStateTypeIds.insert(ovenThingClassId, ovenConnectedStateTypeId);
    m_connectedStateTypeIds.insert(fridgeThingClassId, fridgeConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dryerThingClassId, dryerConnectedStateTypeId);
    m_connectedStateTypeIds.insert(coffeeMakerThingClassId, coffeeMakerConnectedStateTypeId);
    m_connectedStateTypeIds.insert(dishwasherThingClassId, dishwasherConnectedStateTypeId);
    m_connectedStateTypeIds.insert(washerThingClassId, washerConnectedStateTypeId);
    m_connectedStateTypeIds.insert(cookTopThingClassId, cookTopConnectedStateTypeId);
    m_connectedStateTypeIds.insert(cleaningRobotThingClassId, cleaningRobotConnectedStateTypeId);
    m_connectedStateTypeIds.insert(hoodThingClassId, hoodConnectedStateTypeId);

    m_mieleDeviceTypeLabelToThingClassId.insert("oven", ovenThingClassId);
    m_mieleDeviceTypeLabelToThingClassId.insert("washing machine", washerThingClassId);
    m_mieleDeviceTypeLabelToThingClassId.insert("dish washer", dishwasherThingClassId);
    m_mieleDeviceTypeLabelToThingClassId.insert("coffee system", coffeeMakerThingClassId);
    m_mieleDeviceTypeLabelToThingClassId.insert("tumber dryer", dryerThingClassId);
    m_mieleDeviceTypeLabelToThingClassId.insert("refrigerator", fridgeThingClassId);

}

void IntegrationPluginMiele::startPairing(ThingPairingInfo *info)
{
    if (info->thingClassId() == mieleAccountThingClassId) {
        qCDebug(dcMiele()) << "Start pairing Miele account";
        Miele *miele = createMieleConnection();
        if (!miele) {
            info->finish(Thing::ThingErrorAuthenticationFailure);
            return;
        }
        QUrl url = miele->getLoginUrl(QUrl("https://127.0.0.1:8888"));
        m_setupMieleConnections.insert(info->thingId(), miele);
        info->setOAuthUrl(url);
        info->finish(Thing::ThingErrorNoError);
    } else {
        qCWarning(dcMiele()) << "Unhandled pairing metod!";
        info->finish(Thing::ThingErrorCreationMethodNotSupported);
    }
}

void IntegrationPluginMiele::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    Q_UNUSED(username);

    if (info->thingClassId() == mieleAccountThingClassId) {
        qCDebug(dcMiele()) << "Confirm pairing Miele account";
        QUrl url(secret);
        QUrlQuery query(url);
        QByteArray authorizationCode = query.queryItemValue("code").toLocal8Bit();

        Miele *miele = m_setupMieleConnections.value(info->thingId());
        if (!miele) {
            qWarning(dcMiele()) << "No Miele connection found for thing:"  << info->thingName();
            m_setupMieleConnections.remove(info->thingId());
            info->finish(Thing::ThingErrorAuthenticationFailure);
            return;
        }
        qCDebug(dcMiele()) << "Authorization code" << authorizationCode;
        miele->getAccessTokenFromAuthorizationCode(authorizationCode);
        connect(info, &ThingPairingInfo::aborted, miele, &Miele::deleteLater);
        connect(miele, &Miele::receivedRefreshToken, info, [info, this](const QByteArray &refreshToken){
            qCDebug(dcMiele()) << "Token:" << refreshToken;

            pluginStorage()->beginGroup(info->thingId().toString());
            pluginStorage()->setValue("refresh_token", refreshToken);
            pluginStorage()->endGroup();

            info->finish(Thing::ThingErrorNoError);
        });
    } else {
        Q_ASSERT_X(false, "confirmPairing", QString("Unhandled thingClassId: %1").arg(info->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginMiele::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (thing->thingClassId() == mieleAccountThingClassId) {
        qCDebug(dcMiele()) << "Setup Miele Account";
        Miele *miele;

        if (m_mieleConnections.contains(thing)) {
            qCDebug(dcMiele()) << "Setup after reconfiguration, cleaning up";
            m_mieleConnections.take(thing)->deleteLater();
        }

        if (m_setupMieleConnections.keys().contains(thing->id())) {
            //Fresh device setup, has already a fresh access token
            qCDebug(dcMiele()) << "Miele OAuth setup complete";
            miele = m_setupMieleConnections.take(thing->id());
            m_mieleConnections.insert(thing, miele);
            info->finish(Thing::ThingErrorNoError);
        } else {
            // device loaded from the device database, needs a new access token;
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();
            if (refreshToken.isEmpty()) {
                info->finish(Thing::ThingErrorAuthenticationFailure, tr("Refresh token is not available."));
                return;
            }
            miele = createMieleConnection();
            if (!miele) {
                return info->finish(Thing::ThingErrorSetupFailed);
            }
            miele->getAccessTokenFromRefreshToken(refreshToken);
            connect(miele, &Miele::receivedAccessToken, info, [this, info, miele] {
                m_mieleConnections.insert(info->thing(), miele);
               info->finish(Thing::ThingErrorNoError);
            });
            connect(miele, &Miele::receivedRefreshToken, info, [info, thing, this](const QByteArray &refreshToken){
                pluginStorage()->beginGroup(thing->id().toString());
                pluginStorage()->setValue("refresh_token", refreshToken);
                pluginStorage()->endGroup();
                info->finish(Thing::ThingErrorNoError);
            });
            connect(info, &ThingSetupInfo::aborted, miele, &Miele::deleteLater);
            connect(miele, &Miele::devicesFound, this, &IntegrationPluginMiele::onDevicesFound);
            connect(miele, &Miele::deviceStateReceived, this, &IntegrationPluginMiele::onDeviceStateReceived);
        }
    } else if (m_idParamTypeIds.contains(thing->thingClassId())) {
        Thing *parentThing = myThings().findById(thing->parentId());
        if (parentThing->setupComplete()) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            connect(parentThing, &Thing::setupStatusChanged, info, [parentThing, info]{
                if (parentThing->setupComplete()) {
                    info->finish(Thing::ThingErrorNoError);
                }
            });
        }
    } else {
        Q_ASSERT_X(false, "setupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }

}

void IntegrationPluginMiele::postSetupThing(Thing *thing)
{
    qCDebug(dcMiele()) << "Post setup thing" << thing->name();
    if (!m_pluginTimer15min) {
        qCDebug(dcMiele()) << "Registering plugin timer";
        m_pluginTimer15min = hardwareManager()->pluginTimerManager()->registerTimer(60 * 1);
        connect(m_pluginTimer15min, &PluginTimer::timeout, this, [this]() {
            qCDebug(dcMiele()) << "Plugin timer triggered";
            Q_FOREACH (Thing *thing, myThings().filterByThingClassId(mieleAccountThingClassId)) {
                Miele *miele = m_mieleConnections.value(thing);
                if (!miele) {
                    qWarning(dcMiele()) << "No Miele account found for" << thing->name();
                    continue;
                }
                miele->getDevicesShort();
                Q_FOREACH (Thing *childThing, myThings().filterByParentId(thing->id())) {
                    QString deviceId = childThing->paramValue(m_idParamTypeIds.value(childThing->thingClassId())).toString();                    
                    miele->getDeviceState(deviceId);
                }
            }
        });
    }

    if (thing->thingClassId() == mieleAccountThingClassId) {
        Miele *miele = m_mieleConnections.value(thing);
        miele->getDevicesShort();
        //miele->connectEventStream();
        thing->setStateValue(mieleAccountConnectedStateTypeId, true);
        thing->setStateValue(mieleAccountLoggedInStateTypeId, true);
        //TBD Set user name
    } else if (m_idParamTypeIds.contains(thing->thingClassId())) {
        Thing *parentThing = myThings().findById(thing->parentId());
        if (!parentThing)
            qCWarning(dcMiele()) << "Could not find parent with Id" << thing->parentId().toString();
        Miele *miele = m_mieleConnections.value(parentThing);
        QString deviceId = thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString();
        if (!miele) {
            qCWarning(dcMiele()) << "Could not find HomeConnect connection for thing" << thing->name();
        } else {
            miele->getDeviceState(deviceId);
        }
    } else {
        Q_ASSERT_X(false, "postSetupThing", QString("Unhandled thingClassId: %1").arg(thing->thingClassId().toString()).toUtf8());
    }
}

void IntegrationPluginMiele::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();
    Miele *miele = m_mieleConnections.value(myThings().findById(thing->parentId()));
    if (!miele) {
        return info->finish(Thing::ThingErrorHardwareNotAvailable);
    }
    QString deviceId = thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString();

    if (thing->thingClassId() == fridgeThingClassId) {
        if (action.actionTypeId() == fridgeTargetTemperatureActionTypeId) {
            double targetTemp = action.param(fridgeTargetTemperatureActionTargetTemperatureParamTypeId).value().toDouble();
            int iTargetTemp = floor(targetTemp);
            qCDebug(dcMiele()) << "Setting fridge temp: " << iTargetTemp;
            miele->setTargetTemperature(deviceId, 1, iTargetTemp);
            info->finish(Thing::ThingErrorNoError);
            thing->setStateValue(fridgeTargetTemperatureStateTypeId, iTargetTemp);
        }
    } else if (thing->thingClassId() == ovenThingClassId) {

    } else {

    }
}

void IntegrationPluginMiele::thingRemoved(Thing *thing)
{
    qCDebug(dcMiele) << "Delete " << thing->name();

    if (thing->thingClassId() == mieleAccountThingClassId) {
        m_mieleConnections.take(thing)->deleteLater();
    }

    if (myThings().empty()) {
        if (m_pluginTimer15min) {
            hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer15min);
            m_pluginTimer15min = nullptr;
        }
    }
}

Miele *IntegrationPluginMiele::createMieleConnection()
{
    QByteArray clientId = configValue(mielePluginCustomClientIdParamTypeId).toByteArray();
    QByteArray clientSecret = configValue(mielePluginCustomClientSecretParamTypeId).toByteArray();
    if (clientId.isEmpty() || clientSecret.isEmpty()) {
        clientId = apiKeyStorage()->requestKey("miele").data("clientId");
        clientSecret = apiKeyStorage()->requestKey("miele").data("clientSecret");

        if (clientId.isEmpty() || clientSecret.isEmpty()) {
            qCWarning(dcMiele()) << "No API credentials available, cannot create Miele connection";
            return nullptr;
        } else {
            qCDebug(dcMiele()) << "Using api credentials from API key provider";
        }
    } else {
        qCDebug(dcMiele()) << "Using custom api credentials";
    }
    Miele *miele = new Miele(hardwareManager()->networkManager(), clientId, clientSecret, "en", this);
    connect(miele, &Miele::destroyed, this, [this, miele] {
        m_setupMieleConnections.remove(m_setupMieleConnections.key(miele));
        m_mieleConnections.remove(m_mieleConnections.key(miele));
    });
    return miele;
}

void IntegrationPluginMiele::onConnectionChanged(bool connected)
{
    Miele *miele = static_cast<Miele *>(sender());
    Thing *thing = m_mieleConnections.key(miele);
    if (!thing)
        return;
    thing->setStateValue(mieleAccountConnectedStateTypeId, connected);
    if (!connected) {
        Q_FOREACH(Thing *child, myThings().filterByParentId(thing->id())) {
            child->setStateValue(m_connectedStateTypeIds.value(child->thingClassId()), connected);
        }
    }
}

void IntegrationPluginMiele::onAuthenticationStatusChanged(bool authenticated)
{
    Miele *mieleConnection = static_cast<Miele *>(sender());
    if (m_asyncSetup.contains(mieleConnection)) {
        ThingSetupInfo *info = m_asyncSetup.take(mieleConnection);
        if (authenticated) {
            m_mieleConnections.insert(info->thing(), mieleConnection);
            info->finish(Thing::ThingErrorNoError);
        } else {
            mieleConnection->deleteLater();
            info->finish(Thing::ThingErrorHardwareFailure);
        }
    } else {
        Thing *thing = m_mieleConnections.key(mieleConnection);
        if (!thing)
            return;

        thing->setStateValue(mieleAccountLoggedInStateTypeId, authenticated);
        if (!authenticated) {
            //refresh access token needs to be refreshed
            pluginStorage()->beginGroup(thing->id().toString());
            QByteArray refreshToken = pluginStorage()->value("refresh_token").toByteArray();
            pluginStorage()->endGroup();
            mieleConnection->getAccessTokenFromRefreshToken(refreshToken);
        }
    }
}

void IntegrationPluginMiele::onRequestExecuted(QUuid requestId, bool success)
{
    if (m_pendingActions.contains(requestId)) {
        ThingActionInfo *info = m_pendingActions.value(requestId);
        if (success) {
            info->finish(Thing::ThingErrorNoError);
        } else {
            info->finish(Thing::ThingErrorHardwareNotAvailable);
        }
    }
}

void IntegrationPluginMiele::onDevicesFound(QList<Miele::DeviceShort> devices)
{
    qCDebug(dcMiele()) << "Found " << devices.size() << " devices";
    Miele *miele = static_cast<Miele*>(sender());

    if(m_mieleConnections.values().contains(miele)) {
        Thing *parentDevice = myThings().findById(m_mieleConnections.key(miele)->id());
        qCDebug(dcMiele()) << devices.size() << " devices received for prent: " << parentDevice->id();

        ThingDescriptors descriptors;
        foreach(Miele::DeviceShort ds, devices) {
            qCDebug(dcMiele()) << "Setting up device: " << ds.type << " " << ds.name << " " << ds.state;
            if (!m_mieleDeviceTypeLabelToThingClassId.contains(ds.type.toLower())) {
                qCDebug(dcMiele()) << "Device type [" << ds.type << "] is not supported!";
                continue;
            }
            ThingClassId deviceClassId = m_mieleDeviceTypeLabelToThingClassId.value(ds.type.toLower());
            ThingDescriptor descriptor(deviceClassId, ds.name, "Appliance type: " + ds.type, parentDevice->id());
            ParamList params;
            params.append(Param(m_idParamTypeIds.value(deviceClassId), ds.fabNumber));
            if (myThings().findByParams(params)) {
                continue;
            }
            descriptor.setParams(params);
            descriptors.append(descriptor);
        }
        emit autoThingsAppeared(descriptors);

    } else {
        // TODO: improve debug message
        qCWarning(dcMiele()) << "Miele connection not linked to a thingId";
    }
}


void IntegrationPluginMiele::onDeviceStateReceived(const QString &deviceId, const QVariantMap &deviceState)
{
    Miele *miele = static_cast<Miele*>(sender());
    if (!m_mieleConnections.values().contains(miele)) {
        qCWarning(dcMiele()) << "Can't find Miele connection!";
        return;
    }

    Thing *parentDevice = myThings().findById(m_mieleConnections.key(miele)->id());
    if (!parentDevice) {
        qCWarning(dcMiele()) << "Can't find parent thing!";
        return;
    }

    foreach(Thing *thing, myThings().filterByParentId(parentDevice->id())) {
        if (thing->paramValue(m_idParamTypeIds.value(thing->thingClassId())).toString() == deviceId) {
            qCDebug(dcMiele()) << "Received state for: " << thing->name();
            setThingState(thing, deviceState);
            break;
        }
    }

}

void IntegrationPluginMiele::setThingState(Thing *thing, const QVariantMap &state)
{
    qCDebug(dcMiele()) << "Setting device state: " << thing->name();
    if (thing->thingClassId() == fridgeThingClassId) {
        qCDebug(dcMiele()) << "Setting state for fridge!";

        bool connected = state.value("status").toMap().value("value_raw").toInt() == Miele::StatusNotConnected
                       ? false
                       : true;
        thing->setStateValue(m_connectedStateTypeIds.value(thing->thingClassId()), connected);

        // Fridges should have only one temperature zone
        QVariant fridgeTempZone = state.value("temperature").toList().first();
        double fridgeTemp = fridgeTempZone.toMap().value("value_raw").toDouble();
        thing->setStateValue(fridgeTemperatureStateTypeId, fridgeTemp / 100);

        QVariant fridgeTargetTempZone = state.value("targetTemperature").toList().first();
        double fridgeTargetTemp = fridgeTargetTempZone.toMap().value("value_raw").toDouble();
        thing->setStateValue(fridgeTargetTemperatureStateTypeId, fridgeTargetTemp / 100);

    } else {
        qCWarning(dcMiele()) << "Device " << thing->name() << " is not yet supported!";
    }
}
