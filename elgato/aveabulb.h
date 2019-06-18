/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016-2018 Simon Stürz <simon.stuerz@guh.io>              *
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

#ifndef AVEABULB_H
#define AVEABULB_H

#include <QObject>
#include <QQueue>
#include <QColor>

#include "typeutils.h"
#include "devices/device.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

static QBluetoothUuid colorServiceUuid  = QBluetoothUuid(QUuid("f815e810-456c-6761-746f-4d756e696368"));
static QBluetoothUuid colorCharacteristicUuid = QBluetoothUuid(QUuid("f815e811-456c-6761-746f-4d756e696368"));

static QBluetoothUuid imageServiceUuid  = QBluetoothUuid(QUuid("f815e500-456c-6761-746f-4d756e696368"));

class AveaBulb : public QObject
{
    Q_OBJECT
public:
    enum ColorMessage {
        ColorMessageColor = 0x35,
        ColorMessageBrightness = 0x57,
        ColorMessageName = 0x58
    };
    Q_ENUM(ColorMessage)

    explicit AveaBulb(Device *device, BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent = nullptr);

    Device *device();
    BluetoothLowEnergyDevice *bluetoothDevice();

    bool setPower(bool power);
    bool setBulbName(const QString &name);
    bool setColor(const QColor &color);
    bool setBrightness(int percentage);
    bool setFade(int fade);

    bool setWhite(int white);
    bool setRed(int red);
    bool setGreen(int green);
    bool setBlue(int blue);

    bool loadValues();
    bool syncColor();

private:
    Device *m_device = nullptr;
    BluetoothLowEnergyDevice *m_bluetoothDevice = nullptr;

    QString m_bulbName;
    QColor m_color;
    int m_brightness = 0;
    int m_fade = 0;
    int m_white = 0;
    int m_red = 0;
    int m_green = 0;
    int m_blue = 0;

    quint16 scaleColorValueUp(int colorValue);
    int scaleColorValueDown(quint16 colorValue);

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
