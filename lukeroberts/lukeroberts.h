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

#ifndef LUKEROBERTS_H
#define LUKEROBERTS_H

#include <QObject>
#include <QTimer>
#include <QBluetoothUuid>
#include <QByteArray>

#include "typeutils.h"
#include "hardware/bluetoothlowenergy/bluetoothlowenergydevice.h"

static QBluetoothUuid customControlServiceUuid             = QBluetoothUuid(QUuid("​44092840-0567-11E6-B862-0002A5D5C51B"));
static QBluetoothUuid externalApiEndpointCharacteristicUuid       = QBluetoothUuid(QUuid("44092842-0567-11E6-B862-0002A5D5C51B"));

inline QByteArray &operator<<(QByteArray &l, quint8 r)
{
    l.append(r);
    return l;
}

inline QByteArray &operator<<(QByteArray &l, quint16 r)
{
    return l<<quint8(r>>8)<<quint8(r);
}

inline QByteArray &operator<<(QByteArray &l, quint32 r)
{
    return l<<quint16(r>>16)<<quint16(r);
}

class LukeRoberts : public QObject
{
    Q_OBJECT
public:
enum StatusCodes {
    StatusCodeSuccess           = 0x00,
    StatusCodeInvalidParameters = 0x81,
    StatusCodeInvalidId         = 0x84,
    StatusCodeInvalidVersion    = 0x87,
    StatusCodeBadCommand        = 0xbc,
    StatusCodeForbidden         = 0xfc,
    StatusCodeFilteredDuplicate = 0x88
};

enum Opcode {
    OpcodePing,
    OpcodeQueryScene,
    OpcodeImmediateLight,
    OpcodeDuration,
    OpcodeBrightness,
    OpcodeColorTemperature,
    OpcodeSelectScene,
    OpcodeNextSceneByBrightness,
    OpcodeAdjustColorTemperature,
    OpcodeRelativeBrightness
};

struct Scene {
    int8_t id;
    QString name;
};
    explicit LukeRoberts(BluetoothLowEnergyDevice *bluetoothDevice);

    BluetoothLowEnergyDevice *bluetoothDevice();
    void ping();
    void queryScene(uint8_t id);
    void getSceneList();
    void setImmediateLight(uint16_t duration, uint8_t saturation, uint16_t hue, uint8_t brightness);
    void modifyBrightness(uint8_t percent);
    void modifyColorTemperature(uint16_t kelvin);
    void selectScene(uint8_t id);
    void setNextSceneByBrightness(int8_t direction);
    void adjustColorTemperature(uint16_t kelvinIncrement);
    void setRelativeBrightness(uint8_t percent);


private:
    BluetoothLowEnergyDevice *m_bluetoothDevice = nullptr;

    QLowEnergyService *m_deviceInfoService = nullptr;
    QLowEnergyService *m_controlService = nullptr;      //Luke Roberts Custom control service
    QLowEnergyService *m_dfuService = nullptr;

    QLowEnergyCharacteristic m_deviceInfoCharacteristic;
    QLowEnergyCharacteristic m_externalApiEndpoint;

    QList<Scene> m_sceneList;

    void printService(QLowEnergyService *service);
    void onLongPressTimer();

signals:
    void connectedChanged(bool connected);
    void deviceInformationChanged(const QString &firmwareRevision, const QString &hardwareRevision, const QString &softwareRevision);
    void deviceInitializationFinished(bool success);
    void statusCodeReveiced(StatusCodes statusCode);
    void sceneListReceived(QList<Scene> scenes);

private slots:
    void onConnectedChanged(bool connected);
    void onServiceDiscoveryFinished();

    void onDeviceInfoServiceStateChanged(const QLowEnergyService::ServiceState &state);

    void onControlServiceChanged(const QLowEnergyService::ServiceState &state);
    void onControlServiceCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
};

#endif // LUKEROBERTS_H
