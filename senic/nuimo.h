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

#ifndef NUIMO_H
#define NUIMO_H

#include <QObject>
#include <QTimer>
#include <QBluetoothUuid>

#include "typeutils.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

class Nuimo : public QObject
{
    Q_OBJECT
public:
    enum SwipeDirection {
        SwipeDirectionLeft,
        SwipeDirectionRight,
        SwipeDirectionUp,
        SwipeDirectionDown
    };

    enum MatrixType {
        MatrixTypeUp,
        MatrixTypeDown,
        MatrixTypeLeft,
        MatrixTypeRight,
        MatrixTypePlay,
        MatrixTypePause,
        MatrixTypeStop,
        MatrixTypeMusic,
        MatrixTypeHeart,
        MatrixTypeNext,
        MatrixTypePrevious,
        MatrixTypeCircle,
        MatrixTypeFilledCircle,
        MatrixTypeLight
    };

    explicit Nuimo(BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent = nullptr);

    BluetoothLowEnergyDevice *bluetoothDevice();
    void showImage(const MatrixType &matrixType);
    void setLongPressTime(int milliSeconds);

private:
    BluetoothLowEnergyDevice *m_bluetoothDevice = nullptr;

    QLowEnergyService *m_deviceInfoService = nullptr;
    QLowEnergyService *m_batteryService = nullptr;
    QLowEnergyService *m_inputService = nullptr;
    QLowEnergyService *m_ledMatrixService = nullptr;

    QLowEnergyCharacteristic m_deviceInfoCharacteristic;
    QLowEnergyCharacteristic m_batteryCharacteristic;
    QLowEnergyCharacteristic m_ledMatrixCharacteristic;
    QLowEnergyCharacteristic m_inputButtonCharacteristic;
    QLowEnergyCharacteristic m_inputSwipeCharacteristic;
    QLowEnergyCharacteristic m_inputRotationCharacteristic;

    uint m_rotationValue;
    QTimer *m_longPressTimer = nullptr;
    int m_longPressTime = 500;

    void showMatrix(const QByteArray &matrix, const int &seconds);
    void printService(QLowEnergyService *service);
    void onLongPressTimer();

signals:
    void connectedChanged(bool connected);
    void buttonPressed();
    void buttonLongPressed();
    void batteryValueChanged(const uint &percentage);
    void swipeDetected(const SwipeDirection &direction);
    void rotationValueChanged(const uint &value);
    void deviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision);

private slots:
    void onConnectedChanged(const bool &connected);
    void onServiceDiscoveryFinished();

    void onDeviceInfoServiceStateChanged(const QLowEnergyService::ServiceState &state);

    void onBatteryServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onBatteryCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    void onInputServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onInputCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    void onLedMatrixServiceStateChanged(const QLowEnergyService::ServiceState &state);

};

#endif // NUIMO_H
