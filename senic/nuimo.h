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
    QLowEnergyService *m_inputService = nullptr;
    QLowEnergyService *m_ledMatrixService = nullptr;

    QLowEnergyCharacteristic m_deviceInfoCharacteristic;
    QLowEnergyCharacteristic m_ledMatrixCharacteristic;
    QLowEnergyCharacteristic m_inputButtonCharacteristic;
    QLowEnergyCharacteristic m_inputSwipeCharacteristic;
    QLowEnergyCharacteristic m_inputRotationCharacteristic;

    uint m_rotationValue;
    QTimer *m_longPressTimer = nullptr;
    int m_longPressTime = 2000; //default value 2 seconds

    void showMatrix(const QByteArray &matrix, const int &seconds);
    void printService(QLowEnergyService *service);
    void onLongPressTimer();

signals:
    void connectedChanged(bool connected);
    void buttonPressed();
    void buttonLongPressed();
    void swipeDetected(const SwipeDirection &direction);
    void rotationValueChanged(const uint &value);
    void deviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision);
    void deviceInitializationFinished(bool success);

private slots:
    void onConnectedChanged(bool connected);
    void onServiceDiscoveryFinished();

    void onDeviceInfoServiceStateChanged(const QLowEnergyService::ServiceState &state);

    void onInputServiceStateChanged(const QLowEnergyService::ServiceState &state);
    void onInputCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);

    void onLedMatrixServiceStateChanged(const QLowEnergyService::ServiceState &state);

};

#endif // NUIMO_H
