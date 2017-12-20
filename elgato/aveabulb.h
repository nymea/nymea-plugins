/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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

#ifndef AVEABULB_H
#define AVEABULB_H

#include <QObject>
#include <QQueue>
#include <QColor>

#include "typeutils.h"
#include "plugin/device.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

static QBluetoothUuid colorServiceUuid  = QBluetoothUuid(QUuid("f815e810-456c-6761-746f-4d756e696368"));
static QBluetoothUuid imageServiceUuid  = QBluetoothUuid(QUuid("f815e500-456c-6761-746f-4d756e696368"));

class AveaBulb : public QObject
{
    Q_OBJECT
public:
    explicit AveaBulb(Device *device, BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent = nullptr);

    Device *device();
    BluetoothLowEnergyDevice *bluetoothDevice();

    bool setColor(const QColor &color);

private:
    Device *m_device;
    BluetoothLowEnergyDevice *m_bluetoothDevice;

private:
    QLowEnergyService *m_colorService = nullptr;
    QLowEnergyService *m_imageService = nullptr;

    QLowEnergyCharacteristic m_imageCharacteristic;
    QLowEnergyCharacteristic m_colorCharacteristic;

private slots:
    void onConnectedChanged(const bool &connected);
    void onServiceDiscoveryFinished();

    // Color service
    void onColorServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onColorServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

};

#endif // AVEABULB_H
