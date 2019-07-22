/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *                                                                         *
 *  This file is part of nymea.                                            *
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
    // Custom mail
    if(device->deviceClassId() == customMailDeviceClassId) {
        SmtpClient *smtpClient = new SmtpClient(this);
        smtpClient->setHost(device->paramValue(customMailDeviceSmtpParamTypeId).toString());
        smtpClient->setPort(static_cast<quint16>(device->paramValue(customMailDevicePortParamTypeId).toUInt()));
        smtpClient->setUser(device->paramValue(customMailDeviceCustomUserParamTypeId).toString());

        // TODO: use cryptography to save password not as plain text
        smtpClient->setPassword(device->paramValue(customMailDeviceCustomPasswordParamTypeId).toString());

        if(device->paramValue(customMailDeviceAuthenticationParamTypeId).toString() == "PLAIN") {
            smtpClient->setAuthenticationMethod(SmtpClient::AuthenticationMethodPlain);
        } else if(device->paramValue(customMailDeviceAuthenticationParamTypeId).toString() == "LOGIN") {
            smtpClient->setAuthenticationMethod(SmtpClient::AuthenticationMethodLogin);
        } else {
            return DeviceManager::DeviceSetupStatusFailure;
        }

        if(device->paramValue(customMailDeviceEncryptionParamTypeId).toString() == "NONE") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeNone);
        } else if(device->paramValue(customMailDeviceEncryptionParamTypeId).toString() == "SSL") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeSSL);
        } else if(device->paramValue(customMailDeviceEncryptionParamTypeId).toString() == "TLS") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeTLS);
        } else {
            return DeviceManager::DeviceSetupStatusFailure;
        }

        QString recipientsString = device->paramValue(customMailDeviceCustomRecipientParamTypeId).toString();
        QStringList recipients = recipientsString.split(",");

        smtpClient->setRecipients(recipients);
        smtpClient->setSender(device->paramValue(customMailDeviceCustomSenderParamTypeId).toString());

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
    if (device->deviceClassId() == customMailDeviceClassId) {
        if(action.actionTypeId() == customMailNotifyActionTypeId) {
            SmtpClient *smtpClient = m_clients.key(device);
            if (!smtpClient) {
                qCWarning(dcMailNotification()) << "Could not find SMTP client for " << device;
                return DeviceManager::DeviceErrorHardwareNotAvailable;
            }

            smtpClient->sendMail(action.param(customMailNotifyActionTitleParamTypeId).value().toString(),
                                 action.param(customMailNotifyActionBodyParamTypeId).value().toString(),
                                 action.id());
            return DeviceManager::DeviceErrorAsync;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
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
    if (success) {
        qCDebug(dcMailNotification()) << "Email login test successfull";
        emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusSuccess);
    } else {
        qCWarning(dcMailNotification()) << "Email login test failed";
        emit deviceSetupFinished(device, DeviceManager::DeviceSetupStatusFailure);
        if(m_clients.contains(smtpClient)) {
            m_clients.remove(smtpClient);
        }
        smtpClient->deleteLater();
    }
}

void DevicePluginMailNotification::sendMailFinished(const bool &success, const ActionId &actionId)
{
    if (success) {
        qCDebug(dcMailNotification()) << "Email sent successfully";
        emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorNoError);
    } else {
        qCWarning(dcMailNotification()) << "Email sending failed";
        emit actionExecutionFinished(actionId, DeviceManager::DeviceErrorDeviceNotFound);
    }
}
