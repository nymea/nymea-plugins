/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  *
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
        if(fileInfo.fileName()[2] == '-') {
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
