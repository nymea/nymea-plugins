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

#include "integrationpluginmailnotification.h"
#include "plugininfo.h"

#include <integrations/thing.h>

#include <QDebug>
#include <QJsonDocument>
#include <QVariantMap>
#include <QDateTime>

IntegrationPluginMailNotification::IntegrationPluginMailNotification()
{
}

IntegrationPluginMailNotification::~IntegrationPluginMailNotification()
{
}

void IntegrationPluginMailNotification::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter your username and password for the e-mail account."));
}

void IntegrationPluginMailNotification::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    // Just store details, we'll do a test login in setupDevice anyways
    pluginStorage()->beginGroup(info->thingId().toString());
    pluginStorage()->setValue("username", username);
    pluginStorage()->setValue("password", secret);
    pluginStorage()->endGroup();
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginMailNotification::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    // Custom mail
    if(thing->thingClassId() == customMailThingClassId) {
        SmtpClient *smtpClient = new SmtpClient(this);
        smtpClient->setHost(thing->paramValue(customMailThingSmtpParamTypeId).toString());
        smtpClient->setPort(static_cast<quint16>(thing->paramValue(customMailThingPortParamTypeId).toUInt()));

        pluginStorage()->beginGroup(thing->id().toString());
        smtpClient->setUser(pluginStorage()->value("username").toString());
        smtpClient->setPassword(pluginStorage()->value("password").toString());
        pluginStorage()->endGroup();

        if(thing->paramValue(customMailThingAuthenticationParamTypeId).toString() == "PLAIN") {
            smtpClient->setAuthenticationMethod(SmtpClient::AuthenticationMethodPlain);
        } else if(thing->paramValue(customMailThingAuthenticationParamTypeId).toString() == "LOGIN") {
            smtpClient->setAuthenticationMethod(SmtpClient::AuthenticationMethodLogin);
        }

        if(thing->paramValue(customMailThingEncryptionParamTypeId).toString() == "NONE") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeNone);
        } else if(thing->paramValue(customMailThingEncryptionParamTypeId).toString() == "SSL") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeSSL);
        } else if(thing->paramValue(customMailThingEncryptionParamTypeId).toString() == "TLS") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeTLS);
        }

        QString recipientsString = thing->paramValue(customMailThingRecipientParamTypeId).toString();
        QStringList recipients = recipientsString.split(",");

        smtpClient->setRecipients(recipients);
        smtpClient->setSender(thing->paramValue(customMailThingSenderParamTypeId).toString());

        smtpClient->testLogin();
        connect(smtpClient, &SmtpClient::testLoginFinished, info, [this, smtpClient, info, thing](bool success){
            if (!success) {
                qCWarning(dcMailNotification()) << "Email login test failed";
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The email account cannot be accessed. Wrong username or password?"));
                smtpClient->deleteLater();
                return;
            }

            qCDebug(dcMailNotification()) << "Email login test successful.";
            m_clients.insert(smtpClient,thing);
            info->finish(Thing::ThingErrorNoError);
        });

        return;
    }

    info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginMailNotification::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    if (thing->thingClassId() == customMailThingClassId) {
        if(action.actionTypeId() == customMailNotifyActionTypeId) {
            SmtpClient *smtpClient = m_clients.key(thing);
            if (!smtpClient) {
                qCWarning(dcMailNotification()) << "Could not find SMTP client for " << thing;
                return info->finish(Thing::ThingErrorHardwareNotAvailable);
            }

            int id = smtpClient->sendMail(action.param(customMailNotifyActionTitleParamTypeId).value().toString(),
                                 action.param(customMailNotifyActionBodyParamTypeId).value().toString());
            connect(smtpClient, &SmtpClient::sendMailFinished, info, [info, id](bool success, int resultId){
                if (id != resultId) {
                    return; // Not the process we were waiting for
                }
                if (!success) {
                    info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error sending email."));
                    return;
                }
                info->finish(Thing::ThingErrorNoError);
            });
            return;
        }
        return info->finish(Thing::ThingErrorActionTypeNotFound);
    }
    return info->finish(Thing::ThingErrorThingClassNotFound);
}

void IntegrationPluginMailNotification::thingRemoved(Thing *thing)
{
    SmtpClient *smtpClient = m_clients.key(thing);
    m_clients.remove(smtpClient);
    delete smtpClient;
}
