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

#include "devices/device.h"
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

void DevicePluginMailNotification::startPairing(DevicePairingInfo *info)
{
    info->finish(Device::DeviceErrorNoError, QT_TR_NOOP("Please enter your username and password for the e-mail account."));
}

void DevicePluginMailNotification::confirmPairing(DevicePairingInfo *info, const QString &username, const QString &secret)
{
    // Just store details, we'll do a test login in setupDevice anyways
    pluginStorage()->beginGroup(info->deviceId().toString());
    pluginStorage()->setValue("username", username);
    pluginStorage()->setValue("password", secret);
    pluginStorage()->endGroup();
    info->finish(Device::DeviceErrorNoError);
}

void DevicePluginMailNotification::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    // Custom mail
    if(device->deviceClassId() == customMailDeviceClassId) {
        SmtpClient *smtpClient = new SmtpClient(this);
        smtpClient->setHost(device->paramValue(customMailDeviceSmtpParamTypeId).toString());
        smtpClient->setPort(static_cast<quint16>(device->paramValue(customMailDevicePortParamTypeId).toUInt()));

        pluginStorage()->beginGroup(device->id().toString());
        smtpClient->setUser(pluginStorage()->value("username").toString());
        smtpClient->setPassword(pluginStorage()->value("password").toString());
        pluginStorage()->endGroup();

        if(device->paramValue(customMailDeviceAuthenticationParamTypeId).toString() == "PLAIN") {
            smtpClient->setAuthenticationMethod(SmtpClient::AuthenticationMethodPlain);
        } else if(device->paramValue(customMailDeviceAuthenticationParamTypeId).toString() == "LOGIN") {
            smtpClient->setAuthenticationMethod(SmtpClient::AuthenticationMethodLogin);
        }

        if(device->paramValue(customMailDeviceEncryptionParamTypeId).toString() == "NONE") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeNone);
        } else if(device->paramValue(customMailDeviceEncryptionParamTypeId).toString() == "SSL") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeSSL);
        } else if(device->paramValue(customMailDeviceEncryptionParamTypeId).toString() == "TLS") {
            smtpClient->setEncryptionType(SmtpClient::EncryptionTypeTLS);
        }

        QString recipientsString = device->paramValue(customMailDeviceRecipientParamTypeId).toString();
        QStringList recipients = recipientsString.split(",");

        smtpClient->setRecipients(recipients);
        smtpClient->setSender(device->paramValue(customMailDeviceSenderParamTypeId).toString());

        smtpClient->testLogin();
        connect(smtpClient, &SmtpClient::testLoginFinished, info, [this, smtpClient, info, device](bool success){
            if (!success) {
                qCWarning(dcMailNotification()) << "Email login test failed";
                info->finish(Device::DeviceErrorAuthenticationFailure, QT_TR_NOOP("The email account cannot be accessed. Wrong username or password?"));
                smtpClient->deleteLater();
                return;
            }

            qCDebug(dcMailNotification()) << "Email login test successful.";
            m_clients.insert(smtpClient,device);
            info->finish(Device::DeviceErrorNoError);
        });

        return;
    }

    info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginMailNotification::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == customMailDeviceClassId) {
        if(action.actionTypeId() == customMailNotifyActionTypeId) {
            SmtpClient *smtpClient = m_clients.key(device);
            if (!smtpClient) {
                qCWarning(dcMailNotification()) << "Could not find SMTP client for " << device;
                return info->finish(Device::DeviceErrorHardwareNotAvailable);
            }

            int id = smtpClient->sendMail(action.param(customMailNotifyActionTitleParamTypeId).value().toString(),
                                 action.param(customMailNotifyActionBodyParamTypeId).value().toString());
            connect(smtpClient, &SmtpClient::sendMailFinished, info, [info, id](bool success, int resultId){
                if (id != resultId) {
                    return; // Not the process we were waiting for
                }
                if (!success) {
                    info->finish(Device::DeviceErrorHardwareFailure, QT_TR_NOOP("Error sending email."));
                    return;
                }
                info->finish(Device::DeviceErrorNoError);
            });
            return;
        }
        return info->finish(Device::DeviceErrorActionTypeNotFound);
    }
    return info->finish(Device::DeviceErrorDeviceClassNotFound);
}

void DevicePluginMailNotification::deviceRemoved(Device *device)
{
    SmtpClient *smtpClient = m_clients.key(device);
    m_clients.remove(smtpClient);
    delete smtpClient;
}
