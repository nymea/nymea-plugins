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

#ifndef AVEABULB_H
#define AVEABULB_H

#include <QObject>
#include <QQueue>
#include <QColor>

#include "typeutils.h"
#include "integrations/thing.h"
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

    explicit AveaBulb(Thing *thing, BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent = nullptr);

    Thing *thing();
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
    Thing *m_thing = nullptr;
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
