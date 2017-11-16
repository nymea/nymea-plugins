/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stürz <simon.stuerz@guh.io>                   *
 *  Copyright (C) 2016 Bernhard Trinnes <bernhard.trinnes@guh.guru>        *
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
    \page denon.html
    \title Denon
    \brief Plugin for Denon AV

    \ingroup plugins
    \ingroup guh-plugins

    This plug-in supports the
    \l {http://www.denon.de/de/product/hometheater/avreceivers/avrx1000}{Denon AV Amplifier AVR-X1000}

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/denon/deviceplugindenon.json
*/



#include "deviceplugindenon.h"
#include "plugininfo.h"

DevicePluginDenon::DevicePluginDenon()
{

}

DevicePluginDenon::~DevicePluginDenon()
{
    hardwareManager()->pluginTimerManager()->unregisterTimer(m_pluginTimer);
}

void DevicePluginDenon::init()
{
    m_pluginTimer = hardwareManager()->pluginTimerManager()->registerTimer(15);
    connect(m_pluginTimer, &PluginTimer::timeout, this, &DevicePluginDenon::onPluginTimer);
}

DeviceManager::DeviceSetupStatus DevicePluginDenon::setupDevice(Device *device)
{
    qCDebug(dcDenon) << "Setup Denon device" << device->paramValue(AVRX1000IpParamTypeId).toString();

    // Check if we already have a denon device
    if (!myDevices().isEmpty()) {
        qCWarning(dcDenon) << "Could not add denon device. Only one denon device allowed.";
        return DeviceManager::DeviceSetupStatusFailure;
    }

    QHostAddress address(device->paramValue(AVRX1000IpParamTypeId).toString());
    if (address.isNull()) {
        qCWarning(dcDenon) << "Could not parse ip address" << device->paramValue(AVRX1000IpParamTypeId).toString();
        return DeviceManager::DeviceSetupStatusFailure;
    }

    m_device = device;
    m_denonConnection = new DenonConnection(address, 23, this);
    connect(m_denonConnection.data(), &DenonConnection::connectionStatusChanged, this, &DevicePluginDenon::onConnectionChanged);
    connect(m_denonConnection.data(), &DenonConnection::socketErrorOccured, this, &DevicePluginDenon::onSocketError);
    connect(m_denonConnection.data(), &DenonConnection::dataReady, this, &DevicePluginDenon::onDataReceived);

    m_asyncSetups.append(m_denonConnection);
    m_denonConnection->connectDenon();

    return DeviceManager::DeviceSetupStatusAsync;
}

void DevicePluginDenon::deviceRemoved(Device *device)
{
    qCDebug(dcDenon) << "Delete " << device->name();
    if (m_denonConnection.isNull()){
        qCWarning(dcDenon) << "Invalid connection pointer" << device->id().toString();
        return;
    }
    m_device.clear();
    m_denonConnection->disconnectDenon();
    m_denonConnection->deleteLater();
}

DeviceManager::DeviceError DevicePluginDenon::executeAction(Device *device, const Action &action)
{
    qCDebug(dcDenon) << "Execute action" << device->id() << action.id() << action.params();
    if (device->deviceClassId() == AVRX1000DeviceClassId) {

        // check connection state
        if (m_denonConnection.isNull() || !m_denonConnection->connected())
            return DeviceManager::DeviceErrorHardwareNotAvailable;

        // check if the requested action is our "update" action ...
        if (action.actionTypeId() == AVRX1000PowerActionTypeId) {

            // Print information that we are executing now the update action
            qCDebug(dcDenon) << "set power action" << action.id();
            qCDebug(dcDenon) << "power: " << action.param(AVRX1000PowerStateParamTypeId).value().Bool;

            if (action.param(AVRX1000PowerStateParamTypeId).value().toBool() == true){
                QByteArray cmd = "PWON\r";
                qCDebug(dcDenon) << "Execute power: " << action.id() << cmd;
                m_denonConnection->sendData(cmd);
            } else {
                QByteArray cmd = "PWSTANDBY\r";
                qCDebug(dcDenon) << "Execute power: " << action.id() << cmd;
                m_denonConnection->sendData(cmd);
            }

            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000VolumeActionTypeId) {

            QByteArray vol = action.param(AVRX1000VolumeStateParamTypeId).value().toByteArray();
            QByteArray cmd = "MV" + vol + "\r";

            qCDebug(dcDenon) << "Execute volume" << action.id() << cmd;
            m_denonConnection->sendData(cmd);

            return DeviceManager::DeviceErrorNoError;

        } else if (action.actionTypeId() == AVRX1000ChannelActionTypeId) {

            qCDebug(dcDenon) << "Execute update action" << action.id();
            QByteArray channel = action.param(AVRX1000ChannelStateParamTypeId).value().toByteArray();
            QByteArray cmd = "SI" + channel + "\r";

            qCDebug(dcDenon) << "Change to channel:" << cmd;
            m_denonConnection->sendData(cmd);

            return DeviceManager::DeviceErrorNoError;
        }
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorDeviceClassNotFound;
}

void DevicePluginDenon::onPluginTimer()
{
    if (m_denonConnection.isNull())
        return;

    if (!m_denonConnection->connected()) {
        m_denonConnection->connectDenon();
    } else {
        m_denonConnection->sendData("PW?\rSI?\rMV?\r");
    }
}

void DevicePluginDenon::onConnectionChanged()
{
    if (!m_device)
       return;

    // if the device is connected
    if (m_denonConnection->connected()) {
        // and from the first setup
        if (m_asyncSetups.contains(m_denonConnection)) {
            m_asyncSetups.removeAll(m_denonConnection);
            m_denonConnection->sendData("PW?\rSI?\rMV?\r");
            emit deviceSetupFinished(m_device, DeviceManager::DeviceSetupStatusSuccess);
        }
    }

    // Set connection status
    m_device->setStateValue(AVRX1000ConnectedStateTypeId, m_denonConnection->connected());
}

void DevicePluginDenon::onDataReceived(const QByteArray &data)
{
    qCDebug(dcDenon) << "Data received" << data;

    // if there is no device, return
    if (m_device.isNull())
        return;

    if (data.contains("MV") && !data.contains("MAX")){
        int index = data.indexOf("MV");
        int vol = data.mid(index+2, 2).toInt();

        qCDebug(dcDenon) << "Update volume:" << vol;
        m_device->setStateValue(AVRX1000VolumeStateTypeId, vol);
    }

    if (data.contains("SI")) {
        QString cmd = NULL;
        if (data.contains("TUNER")) {
            cmd = "TUNER";
        } else if (data.contains("DVD")) {
            cmd = "DVD";
        } else if (data.contains("BD")) {
            cmd = "BD";
        } else if (data.contains("TV")) {
            cmd = "TV";
        } else if (data.contains("SAT/CBL")) {
            cmd = "SAT/CBL";
        } else if (data.contains("MPLAY")) {
            cmd = "MPLAY";
        } else if (data.contains("GAME")) {
            cmd = "GAME";
        } else if (data.contains("AUX1")) {
            cmd = "AUX1";
        } else if (data.contains("NET")) {
            cmd = "NET";
        } else if (data.contains("PANDORA")) {
            cmd = "PANDORA";
        } else if (data.contains("SIRIUSXM")) {
            cmd = "SIRIUSXM";
        } else if (data.contains("SPOTIFY")) {
            cmd = "SPOTIFY";
        } else if (data.contains("FLICKR")) {
            cmd = "FLICKR";
        } else if (data.contains("FAVORITES")) {
            cmd = "FAVORITES";
        } else if (data.contains("IRADIO")) {
            cmd = "IRADIO";
        } else if (data.contains("SERVER")) {
            cmd = "SERVER";
        } else if (data.contains("USB/IPOD")) {
            cmd = "USB/IPOD";
        } else if (data.contains("IPD")) {
            cmd = "IPD";
        } else if (data.contains("IRP")) {
            cmd = "IRP";
        } else if (data.contains("FVP")) {
            cmd = "FVP";
        }

        qCDebug(dcDenon) << "Update channel:" << cmd;
        m_device->setStateValue(AVRX1000ChannelStateTypeId, cmd);
    }

    if (data.contains("PWON")) {
        qCDebug(dcDenon) << "Update power on";
        m_device->setStateValue(AVRX1000PowerStateTypeId, true);
    } else if (data.contains("PWSTANDBY")) {
        qCDebug(dcDenon) << "Update power off";
        m_device->setStateValue(AVRX1000PowerStateTypeId, false);
    }
}

void DevicePluginDenon::onSocketError()
{
    // if there is no device, return
    if (m_device.isNull())
        return;

    // Check if setup running for this device
    if (m_asyncSetups.contains(m_denonConnection)) {
        qCWarning(dcDenon()) << "Could not add device. The setup failed.";
        emit deviceSetupFinished(m_device, DeviceManager::DeviceSetupStatusFailure);
        // Delete the connection, the device will not be added and
        // the connection will be created in the next setup
        m_denonConnection->deleteLater();
    }
}



