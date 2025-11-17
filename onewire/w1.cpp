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

#include "w1.h"
#include "extern-plugininfo.h"

W1::W1(QObject *parent) :
    QObject(parent)
{

}

QStringList W1::discoverDevices()
{
    QStringList deviceList;

    QDir w1SysFSDir("/sys/bus/w1/devices/");
    if (!w1SysFSDir.exists()) {
        qCDebug(dcOneWire()) << "W1 kernel not loaded";
        return deviceList;
    }
    w1SysFSDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    w1SysFSDir.setSorting(QDir::Name);

    QFileInfoList list = w1SysFSDir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if(fileInfo.fileName().at(2) == '-') {
            qCDebug(dcOneWire()) << "Found one wire device" << fileInfo.filePath();
            deviceList.append(fileInfo.fileName());
        }
    }
    return deviceList;
}

bool W1::interfaceIsAvailable()
{
    QDir w1SysFSDir("/sys/bus/w1/devices/");
    return w1SysFSDir.exists();
}

bool W1::deviceAvailable(const QString &address)
{
   QDir temperatureSensor("/sys/bus/w1/devices/"+address);
   return temperatureSensor.exists();
}

double W1::getTemperature(const QString &address)
{
    QDir temperatureSensor("/sys/bus/w1/devices/"+address);
    if (temperatureSensor.exists()) {
        QFile temperature(temperatureSensor.path() +"/temperature");
        if (!temperature.exists()) {
            qCWarning(dcOneWire()) << "Directory doesn't exist" << temperature.fileName();
        }
        if (!temperature.open(QIODevice::ReadOnly | QIODevice::Text)){
            qCWarning(dcOneWire()) << "Could not open file" << temperature.fileName();
            return 0;
        }
        return temperature.readLine().toInt()/1000.00;
    } else {
        qCWarning(dcOneWire()) << "Could not find device" << temperatureSensor.currentPath();
    }
    return 0;
}
