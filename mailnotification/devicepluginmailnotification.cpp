/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
    \page mailnotification.html
    \title Mail Notification
    \brief Plugin which allows to get mail notification from guh.

    \ingroup plugins
    \ingroup guh-plugins

    The mail notification plugin allows you to send a mail notification from a mail
    account by performing an \l{Action}.

    ATTENTION: The password currently will be saved as plain text in the guh settings file.
    This will be changed soon...

    \chapter Supported services

        \section2 Google Mail
        With the Google Mail Notification you can send a mail with your gmail address to a recipient. The
        username is your mail address (e.g. "chuck.norris@gmail.com"). The recipient will receive the notification
        from your gmail account.

        \section2 Yahoo Mail
        The Yahoo Mail Notification you can send a mail with your yahoo address to a recipient. The username
        is your mail address (e.g. "chuck.norris@yahoo.com"). The recipient will receive the notification
        from your yahoo account.

        \section2 Custom Mail
        With the Custom Mail Notification you can set up a custom SMTP connection. The supported authentification
        methods are ["PLAIN", "LOGIN"], the supported encryption methods are ["NONE", "SSL", "TLS"].

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/mailnotification/devicepluginmailnotification.json
*/

#include "devicepluginmailnotification.h"

#include "plugin/device.h"
#include "devicemanager.h"
#include "plugininfo.h"

#include <QDebug>
#include <QJsonDocument>
#include <QVariantMap>
#include <QDateTime>

DevicePluginMailNotification::DevicePluginMailNotification()
{
}

DevicePluginMailNotification::~DevicePluginMailNotification()
{
}

DeviceManager::DeviceSetupStatus DevicePluginMailNotification::setupDevice(Device *device)
{
    // Google mail
    if(device->deviceClassId() == googleMailDeviceClassId) {
        SmtpClient *smtpClient = new SmtpClient(this);
        smtpClient->setHost("smtp.gmail.com");
        smtpClient->setPort(465);
        smtpClient->setUser(device->paramValue(googleMailUserParamTypeId).toString());
        // TODO: use cryptography to save password not as plain text
        smtpClient->setPassword(device->paramValue(googleMailPasswordParamTypeId).toString());
        smtpClient->setAuthMethod(SmtpClient::AuthMethodLogin);
        smtpClient->setEncryptionType(SmtpClient::EncryptionTypeSSL);
        smtpClient->setSender(device->paramValue(googleMailUserParamTypeId).toString());
        smtpClient->setRecipient(device->paramValue(googleMailRecipientParamTypeId).toString());

        connect(smtpClient, &SmtpClient::testLoginFinished, this, &DevicePluginMailNotification::testLoginFinished);
        connect(smtpClient, &SmtpClient::sendMailFinished, this, &DevicePluginMailNotification::sendMailFinished);
        m_clients.insert(smtpClient,device);

        smtpClient->testLogin();

        return DeviceManager::DeviceSetupStatusAsync;
    }
    // Yahoo mail
    if(device->deviceClassId() == yahooMailDeviceClassId) {
        SmtpClient *smtpClient = new SmtpClient(this);
        smtpClient->setHost("smtp.mail.yahoo.com");
        smtpClient->setPort(465);
        smtpClient->setUser(device->paramValue(yahooMailUserParamTypeId).toString());
        // TODO: use cryptography to save password not as plain text
        smtpClient->setPassword(device->paramValue(yahooMailPasswordParamTypeId).toString());
        smtpClient->setAuthMethod(SmtpClient::AuthMethodLogin);
        smtpClient->setEncryptionType(SmtpClient::EncryptionTypeSSL);
        smtpClient->setSender(device->paramValue(yahooMailUserParamTypeId).toString());
        smtpClient->setRecipient(device->paramValue(yahooMailRecipientParamTypeId).toString());

        connect(smtpClient, &SmtpClient::testLoginFinished, this, &DevicePluginMailNotification::testLoginFinished);
        connect(smtpClient, &SmtpClient::sendMailFinished, this, &DevicePluginMailNotification::sendMailFinished);
        m_clients.insert(smtpClient,device);

        smtpClient->testLogin();

        return DeviceManager::DeviceSetupStatusAsync;
    }
    // Custom mail
    if(device->deviceClassId() == customMailDeviceClassId) {
        SmtpClient *smtpClient = new SmtpClient(this);
        smtpClient->setHost(device->paramValue(customMailSmtpParamTypeId).toString());
        smtpClient->setPort(device->paramValue(customMailPortParamTypeId).toInt());
        smtpClient->setUser(device->paramValue(customMailCustomUserParamTypeId).toString());

        // TODO: use cryptography to save password not as plain text
        smtpClient->setPassword(device->paramValue(customMailCustomPasswordParamTypeId).toString());

        if(device->paramValue(customMailAuthenticationParamTypeId).toString() == "PLAIN") {
            smtpClient->setAuthMethod(SmtpClient::AuthMethodPlain);
        } else if(device->paramValue(customMailAuthenticationParamTypeId).toString() == "LOGIN") {
            smtpClient->setAuthMethod(SmtpClient::AuthMethodLogin);
        } else {
            return DeviceManager::DeviceSetupStatusFailure;
        }

        if(device->paramValue(customMailEncryptionParamTypeId).toString() == "NONE") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeNone);
        } else if(device->paramValue(customMailEncryptionParamTypeId).toString() == "SSL") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeSSL);
        } else if(device->paramValue(customMailEncryptionParamTypeId).toString() == "TLS") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeTLS);
        } else {
            return DeviceManager::DeviceSetupStatusFailure;
        }

        smtpClient->setRecipient(device->paramValue(customMailCustomRecipientParamTypeId).toString());
        smtpClient->setSender(device->paramValue(customMailCustomSenderParamTypeId).toString());

        connect(smtpClient, &SmtpClient::testLoginFinished, this, &DevicePluginMailNotification::testLoginFinished);
        connect(smtpClient, &SmtpClient::sendMailFinished, this, &DevicePluginMailNotification::sendMailFinished);
        m_clients.insert(smtpClient,device);

        smtpClient->testLogin();

        return DeviceManager::DeviceSetupStatusAsync;
    }
    return DeviceManager::DeviceSetupStatusFailure;
}

DeviceManager::DeviceError DevicePluginMailNotification::executeAction(Device *device, const Action &action)
{
    if(action.actionTypeId() == googleMailSendMailActionTypeId) {
        SmtpClient *smtpClient = m_clients.key(device);
        smtpClient->sendMail(action.param(googleMailSubjectParamTypeId).value().toString(), action.param(googleMailBodyParamTypeId).value().toString(), action.id());
        return DeviceManager::DeviceErrorAsync;
    }
    return DeviceManager::DeviceErrorActionTypeNotFound;
}

void DevicePluginMailNotification::deviceRemoved(Device *device)
{
    SmtpClient *smtpClient = m_clients.key(device);
    m_clients.remove(smtpClient);
    delete smtpClient;
}

void DevicePluginMailNotification::testLoginFinished(const bool &success)
{
    SmtpClient *smtpClient = static_cast<SmtpClient*>(sender());
    Device *device = m_clients.value(smtpClient);
    if(success) {
        emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);
    } else {
        emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
        if(m_clients.contains(smtpClient)) {
            m_clients.remove(smtpClient);
        }
        smtpClient->deleteLater();
    }
}

void DevicePluginMailNotification::sendMailFinished(const bool &success, const ActionId &actionId)
{
    if(success) {
        emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorNoError);
    } else {
        emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorDeviceNotFound);
    }
}
