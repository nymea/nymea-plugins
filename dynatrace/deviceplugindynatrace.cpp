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

#include "deviceplugindynatrace.h"
#include "devices/device.h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"

#include <QDebug>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QHostInfo>

DevicePluginDynatrace::DevicePluginDynatrace()
{

}

void DevicePluginDynatrace::discoverDevices(DeviceDiscoveryInfo *info)
{
    if (info->deviceClassId() == ufoDeviceClassId) {

        QHostInfo::lookupHost("ufo.home", this, [info, this](const QHostInfo &host){
            if (host.error() != QHostInfo::NoError) {
                qCDebug(dcDynatrace()) << "Lookup failed:" << host.errorString();
            }

            foreach (QHostAddress address, host.addresses()) {
                qCDebug(dcDynatrace()) << "Found IP address" << address.toString();

                DeviceDescriptor descriptor(ufoDeviceClassId, "Ufo", address.toString());
                ParamList params;

                /*Device *existingDevice = myDevices().findByParams(ParamList() << Param(ufoDeviceIdParamTypeId, ""));
                if (existingDevice) {
                    //For device re-discovery
                    descriptor.setDeviceId(existingDevice->id());
                }*/
                params << Param(ufoDeviceHostParamTypeId, address.toString());
                descriptor.setParams(params);
                info->addDeviceDescriptor(descriptor);
            }
            info->finish(Device::DeviceErrorNoError);
        });
    }
}

void DevicePluginDynatrace::setupDevice(DeviceSetupInfo *info)
{

    if (info->device()->deviceClassId() == ufoDeviceClassId) {
        QHostAddress address = QHostAddress(info->device()->paramValue(ufoDeviceHostParamTypeId).toString());
        QString id = info->device()->paramValue(ufoDeviceIdParamTypeId).toString();
        if(id.isEmpty()) { //Probably a manual device setup
            QUrl url;
            url.setScheme("http");
            url.setHost(address.toString());
            url.setPath("/info", QUrl::ParsingMode::TolerantMode);
            QNetworkRequest request;
            request.setUrl(url);
            QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
            connect(reply, &QNetworkReply::finished, this, [info, reply, this] {
                reply->deleteLater();

                QJsonParseError error;
                QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
                if (error.error != QJsonParseError::NoError) {
                    info->finish(Device::DeviceErrorSetupFailed, error.errorString());
                }

                QString id = data.toVariant().toMap().value("ufoid").toString();
                info->device()->setParamValue(ufoDeviceIdParamTypeId, id);
                info->finish(Device::DeviceErrorNoError);
            });
        } else {
            // Discovery device setup
            info->finish(Device::DeviceErrorNoError);
        }
    }
}

void DevicePluginDynatrace::postSetupDevice(Device *device)
{
    if (device->deviceClassId() == ufoDeviceClassId) {
        QHostAddress address = QHostAddress(device->paramValue(ufoDeviceHostParamTypeId).toString());
        Ufo *ufo = new Ufo(hardwareManager()->networkManager(), address, this);
        m_ufoConnections.insert(device->id(), ufo);
        ufo->resetLogo();
    }

    if(!m_pluginTimer) {
        m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(60);
        connect(m_pluginTimer, &PluginTimer::timeout, this, [this]() {
            //TODO check if device is reachable
        });
    }
}

void DevicePluginDynatrace::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();
    Action action = info->action();

    if (device->deviceClassId() == ufoDeviceClassId) {
        Ufo *ufo = m_ufoConnections.value(device->id());
        if (!ufo)
            return;
        if (action.actionTypeId() == ufoLogoActionTypeId) {
            bool power = action.param(ufoLogoActionLogoParamTypeId).value().toBool();
            device->setStateValue(ufoLogoStateTypeId, power);
            if (power) {
                ufo->resetLogo();
            } else {
                QColor color;
                color.setRgb(0, 0, 0);
                ufo->setLogo(color, color, color, color);
            }
            info->finish(Device::DeviceErrorNoError);

        } else if (action.actionTypeId() == ufoPowerActionTypeId) {
            bool power = action.param(ufoPowerActionPowerParamTypeId).value().toBool();
            device->setStateValue(ufoPowerStateTypeId, power);
            if (power) {
                ufo->setBackgroundColor(true, true, QColor(Qt::white)); //#ffffff
            } else {
                ufo->setBackgroundColor(true, true, QColor(Qt::black)); //#000000
            }
            info->finish(Device::DeviceErrorNoError);

        } else if (action.actionTypeId() == ufoBrightnessActionTypeId) {
            int brightness = action.param(ufoBrightnessActionBrightnessParamTypeId).value().toInt();
            QColor color = QColor(device->stateValue(ufoColorStateTypeId).toString());
            color.setHsv(color.hue(), color.saturation(), brightness);
            ufo->setBackgroundColor(true, true, color);
            info->finish(Device::DeviceErrorNoError);
        } else if (action.actionTypeId() == ufoColorActionTypeId) {
            QColor color = QColor(action.param(ufoColorActionColorParamTypeId).value().toString());
            int brightness = device->stateValue(ufoBrightnessStateTypeId).toInt();
            device->setStateValue(ufoColorStateTypeId, color);
            color.setHsv(color.hue(), color.saturation(), brightness);
            ufo->setBackgroundColor(true, true, color);
            info->finish(Device::DeviceErrorNoError);
        } else {
            qCWarning(dcDynatrace()) << "Execute action: Unhandled actionTypeId";
        }
    } else {
        qCWarning(dcDynatrace()) << "Execute action: Unhandled deviceClass";
    }
}

void DevicePluginDynatrace::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == ufoDeviceClassId) {
        if (m_ufoConnections.contains(device->id())){
            Ufo *ufo = m_ufoConnections.take(device->id());
            ufo->deleteLater();
        }
    }

    if (myDevices().isEmpty() && m_pluginTimer) {
        m_pluginTimer->deleteLater();
        m_pluginTimer = nullptr;
    }
}

void DevicePluginDynatrace::getId(const QHostAddress &address)
{
    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPath("/info", QUrl::ParsingMode::TolerantMode);
    QNetworkRequest request;
    request.setUrl(url);
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, this] {
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument data = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError)
            return;

        QString id = data.toVariant().toMap().value("ufoid").toString();
        if (m_asyncSetup.contains(reply->url().host())) {
            DeviceSetupInfo *info = m_asyncSetup.value(reply->url().host());
            info->finish(Device::DeviceErrorNoError);
        }
    });
}

void DevicePluginDynatrace::onConnectionChanged(bool connected)
{
    Q_UNUSED(connected)
}
