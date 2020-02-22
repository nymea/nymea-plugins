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

#include "devicepluginpiface.h"
#include "types/param.h"
#include "devices/device.h"
#include "plugininfo.h"

DevicePluginPiFace::DevicePluginPiFace()
{

}

void DevicePluginPiFace::discoverDevices(DeviceDiscoveryInfo *info)
{
    DeviceClassId deviceClassId = info->deviceClassId();

    if(myDevices().filterByDeviceClassId(pifaceDeviceClassId)) {
        qCDebug(dcPiFace()) << "not discoverin IO, parent device is not setup";
        info->finish(Device::DeviceErrorHardwareNotAvailable), tr("Please add first the PiFace device");
    }

    if (deviceClassId == inputDeviceClassId) {

        QList<int> usedPins;
        foreach (Device *device, myDevices().findById(inputDeviceClassId)) {

        }

    } else if (deviceClassId == outputDeviceClassId) {

    } else {
        qCWarning(dcPiFace()) << "";
        info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
}

void DevicePluginPiFace::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == pifaceDeviceClassId) {
        //TODO check if there is already a piface device
        int busAddress = device->paramValue(piface).toBool();
        m_piface = new PiFace(this);

    } else if (device->deviceClassId() == inputDeviceClassId || device->deviceClassId() == outputDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
    } else {
        qCWarning(dcPiFace);
        info->finish(Device::DeviceErrorDeviceClassNotFound);
    }
}



void DevicePluginPiFace::deviceRemoved(Device *device)
{
    Q_UNUSED(device)
}

