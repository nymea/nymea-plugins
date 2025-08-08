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

#include "nuimo.h"
#include "extern-plugininfo.h"

#include <QBitArray>
#include <QtEndian>

static QBluetoothUuid ledMatrinxServiceUuid             = QBluetoothUuid(QUuid("f29b1523-cb19-40f3-be5c-7241ecb82fd1"));
static QBluetoothUuid ledMatrixCharacteristicUuid       = QBluetoothUuid(QUuid("f29b1524-cb19-40f3-be5c-7241ecb82fd1"));

static QBluetoothUuid inputServiceUuid                  = QBluetoothUuid(QUuid("f29b1525-cb19-40f3-be5c-7241ecb82fd2"));
static QBluetoothUuid inputSwipeCharacteristicUuid      = QBluetoothUuid(QUuid("f29b1527-cb19-40f3-be5c-7241ecb82fd2"));
static QBluetoothUuid inputRotationCharacteristicUuid   = QBluetoothUuid(QUuid("f29b1528-cb19-40f3-be5c-7241ecb82fd2"));
static QBluetoothUuid inputButtonCharacteristicUuid     = QBluetoothUuid(QUuid("f29b1529-cb19-40f3-be5c-7241ecb82fd2"));


Nuimo::Nuimo(BluetoothLowEnergyDevice *bluetoothDevice, QObject *parent) :
    QObject(parent),
    m_bluetoothDevice(bluetoothDevice)
{
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::connectedChanged, this, &Nuimo::onConnectedChanged);
    connect(m_bluetoothDevice, &BluetoothLowEnergyDevice::servicesDiscoveryFinished, this, &Nuimo::onServiceDiscoveryFinished);

    if (!m_longPressTimer) {
        m_longPressTimer = new QTimer(this);
        m_longPressTimer->setSingleShot(true);
    }
    connect(m_longPressTimer, &QTimer::timeout, this, &Nuimo::onLongPressTimer);
}


BluetoothLowEnergyDevice *Nuimo::bluetoothDevice()
{
    return m_bluetoothDevice;
}

void Nuimo::showImage(const Nuimo::MatrixType &matrixType)
{
    QByteArray matrix;
    int time = 3;
    switch (matrixType) {
    case MatrixTypeUp:
        matrix = QByteArray(
                    "    *    "
                    "   ***   "
                    "  * * *  "
                    " *  *  * "
                    "*   *   *"
                    "    *    "
                    "    *    "
                    "    *    "
                    "    *    ");
        time = 3;
        break;
    case MatrixTypeDown:
        matrix = QByteArray(
                    "    *    "
                    "    *    "
                    "    *    "
                    "    *    "
                    "*   *   *"
                    " *  *  * "
                    "  * * *  "
                    "   ***   "
                    "    *    ");
        time = 3;
        break;
    case MatrixTypeLeft:
        matrix = QByteArray(
                    "    *    "
                    "   *     "
                    "  *      "
                    " *       "
                    "*********"
                    " *       "
                    "  *      "
                    "   *     "
                    "    *    ");
        time = 3;
        break;
    case MatrixTypeRight:
        matrix = QByteArray(
                    "    *    "
                    "     *   "
                    "      *  "
                    "       * "
                    "*********"
                    "       * "
                    "      *  "
                    "     *   "
                    "    *    ");
        time = 3;
        break;
    case MatrixTypePlay:
        matrix = QByteArray(
                    "         "
                    "   *     "
                    "   **    "
                    "   ***   "
                    "   ****  "
                    "   ***   "
                    "   **    "
                    "   *     "
                    "         ");
        time = 3;
        break;
    case MatrixTypePause:
        matrix = QByteArray(
                    "         "
                    "         "
                    "  ** **  "
                    "  ** **  "
                    "  ** **  "
                    "  ** **  "
                    "  ** **  "
                    "         "
                    "         ");
        time = 3;
        break;
    case MatrixTypeStop:
        matrix = QByteArray(
                    "         "
                    "         "
                    "  *****  "
                    "  *****  "
                    "  *****  "
                    "  *****  "
                    "  *****  "
                    "         "
                    "         ");
        time = 3;
        break;
    case MatrixTypeMusic:
        matrix = QByteArray(
                    "  *******"
                    "  *******"
                    "  *     *"
                    "  *     *"
                    "  *     *"
                    "  *     *"
                    " **    **"
                    "***   ***"
                    " *     * ");
        time = 5;
        break;
    case MatrixTypeHeart:
        matrix = QByteArray(
                    "         "
                    "  ** **  "
                    " ******* "
                    "*********"
                    "*********"
                    " ******* "
                    "  *****  "
                    "   ***   "
                    "    *    ");
        time = 5;
        break;
    case MatrixTypeNext:
        matrix = QByteArray(
                    "         "
                    "  *   ** "
                    "  **  ** "
                    "  *** ** "
                    "  ****** "
                    "  *** ** "
                    "  **  ** "
                    "  *   ** "
                    "         ");
        time = 5;
        break;
    case MatrixTypePrevious:
        matrix = QByteArray(
                    "         "
                    "  **   * "
                    "  **  ** "
                    "  ** *** "
                    "  ****** "
                    "  ** *** "
                    "  **  ** "
                    "  **   * "
                    "         ");
        time = 5;
        break;
    case MatrixTypeCircle:
        matrix = QByteArray(
                    "         "
                    "         "
                    "   ***   "
                    "  *   *  "
                    "  *   *  "
                    "  *   *  "
                    "   ***   "
                    "         "
                    "         ");
        time = 5;
        break;
    case MatrixTypeFilledCircle:
        matrix = QByteArray(
                    "         "
                    "         "
                    "   ***   "
                    "  *****  "
                    "  *****  "
                    "  *****  "
                    "   ***   "
                    "         "
                    "         ");
        time = 5;
        break;
    case MatrixTypeLight:
        matrix = QByteArray(
                    "         "
                    "   ***   "
                    "  *   *  "
                    "  *   *  "
                    "  *   *  "
                    "   ***   "
                    "   ***   "
                    "   ***   "
                    "    *    ");
        time = 5;
        break;
    }

    showMatrix(matrix, time);
}

void Nuimo::setLongPressTime(int milliSeconds)
{
    m_longPressTime = milliSeconds;
}

void Nuimo::showMatrix(const QByteArray &matrix, const int &seconds)
{
    if (!m_ledMatrixService)
        return;

    QBitArray bits;
    bits.resize(81);
    for (int i = 0; i < matrix.size(); i++) {
        if (matrix.at(i) != ' ') {
            bits[i] = true;
        }
    }

    QByteArray bytes;
    bytes.resize(bits.count() / 8 + 1);
    bytes.fill(0);
    // Convert from QBitArray to QByteArray
    for(int b = 0; b < bits.count(); ++b)
        bytes[b / 8] = (bytes.at(b / 8) | ((bits[b] ? 1 : 0) << (b % 8)));

    bytes.append(QByteArray::fromHex("FF"));
    quint8 time = quint8(seconds * 10);
    bytes.append((char)time);
    m_ledMatrixService->writeCharacteristic(m_ledMatrixCharacteristic, bytes);
}

void Nuimo::printService(QLowEnergyService *service)
{
    foreach (const QLowEnergyCharacteristic &characteristic, service->characteristics()) {
        qCDebug(dcSenic()) << "    -->" << characteristic.name() << characteristic.uuid().toString() << characteristic.value();
        foreach (const QLowEnergyDescriptor &descriptor, characteristic.descriptors()) {
            qCDebug(dcSenic()) << "        -->" << descriptor.name() << descriptor.uuid().toString() << descriptor.value();
        }
    }
}

void Nuimo::onLongPressTimer()
{
    emit buttonLongPressed();
}



void Nuimo::onConnectedChanged(bool connected)
{
    qCDebug(dcSenic()) << m_bluetoothDevice->name() << m_bluetoothDevice->address().toString() << (connected ? "connected" : "disconnected");

    m_longPressTimer->stop();
    emit connectedChanged(connected);

    if (!connected) {
        // Clean up services
        m_deviceInfoService->deleteLater();

        // FIXME: As of BlueZ 5.48, the Battery service isn't expose in the same way any more.
        // Until that's fixed m_batteryService might never be initialized
        if (m_batteryService) {
            m_batteryService->deleteLater();
        }

        m_ledMatrixService->deleteLater();
        m_inputService->deleteLater();

        m_deviceInfoService = nullptr;
        m_batteryService = nullptr;
        m_ledMatrixService = nullptr;
        m_inputService = nullptr;
    }

}

void Nuimo::onServiceDiscoveryFinished()
{
    qCDebug(dcSenic()) << "Service scan finised";

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (!m_bluetoothDevice->serviceUuids().contains(QBluetoothUuid::ServiceClassUuid::DeviceInformation)) {
#else
    if (!m_bluetoothDevice->serviceUuids().contains(QBluetoothUuid::DeviceInformation)) {
#endif
        qCWarning(dcSenic()) << "Device Information service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        emit deviceInitializationFinished(false);
        return;
    }

    // FIXME: As of BlueZ 5.48, the Battery service isn't expose in the same way any more.
    // For now we're deactivating this check to make the Nuimo still work despite the broken battery info
//    if (!m_bluetoothDevice->serviceUuids().contains(QBluetoothUuid::BatteryService)) {
//        qCWarning(dcSenic()) << "Battery service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
//        emit deviceInitializationFinished(false);
//        return;
//    }

    if (!m_bluetoothDevice->serviceUuids().contains(ledMatrinxServiceUuid)) {
        qCWarning(dcSenic()) << "Led matrix service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        emit deviceInitializationFinished(false);
        return;
    }

    if (!m_bluetoothDevice->serviceUuids().contains(inputServiceUuid)) {
        qCWarning(dcSenic()) << "Input service not found for device" << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        emit deviceInitializationFinished(false);
        return;
    }

    // Device info service
    if (!m_deviceInfoService) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        m_deviceInfoService = m_bluetoothDevice->controller()->createServiceObject(QBluetoothUuid::ServiceClassUuid::DeviceInformation, this);
#else
        m_deviceInfoService = m_bluetoothDevice->controller()->createServiceObject(QBluetoothUuid::DeviceInformation, this);
#endif
        if (!m_deviceInfoService) {
            qCWarning(dcSenic()) << "Could not create thing info service.";
            emit deviceInitializationFinished(false);
            return;
        }

        connect(m_deviceInfoService, &QLowEnergyService::stateChanged, this, &Nuimo::onDeviceInfoServiceStateChanged);

        if (m_deviceInfoService->state() == QLowEnergyService::DiscoveryRequired) {
            m_deviceInfoService->discoverDetails();
        }
    }

    // Battery service
    if (!m_batteryService) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        m_batteryService = m_bluetoothDevice->controller()->createServiceObject(QBluetoothUuid::ServiceClassUuid::BatteryService, this);
#else
        m_batteryService = m_bluetoothDevice->controller()->createServiceObject(QBluetoothUuid::BatteryService, this);
#endif
        if (!m_batteryService) {
            qCWarning(dcSenic()) << "Could not create battery service.";

            // FIXME: As of BlueZ 5.48, the Battery service isn't expose in the same way any more.
            // For now we're deactivating this check to make the Nuimo still work despite the broken battery info
//            emit deviceInitializationFinished(false);
//            return;
        } else {
            connect(m_batteryService, &QLowEnergyService::stateChanged, this, &Nuimo::onBatteryServiceStateChanged);
            connect(m_batteryService, &QLowEnergyService::characteristicChanged, this, &Nuimo::onBatteryCharacteristicChanged);

            if (m_batteryService->state() == QLowEnergyService::DiscoveryRequired) {
                m_batteryService->discoverDetails();
            }
        }
    }

    // Input service
    if (!m_inputService) {
        m_inputService = m_bluetoothDevice->controller()->createServiceObject(inputServiceUuid, this);
        if (!m_inputService) {
            qCWarning(dcSenic()) << "Could not create input service.";
            emit deviceInitializationFinished(false);
            return;
        }

        connect(m_inputService, &QLowEnergyService::stateChanged, this, &Nuimo::onInputServiceStateChanged);
        connect(m_inputService, &QLowEnergyService::characteristicChanged, this, &Nuimo::onInputCharacteristicChanged);

        if (m_inputService->state() == QLowEnergyService::DiscoveryRequired) {
            m_inputService->discoverDetails();
        }
    }

    // Input service
    if (!m_ledMatrixService) {
        m_ledMatrixService = m_bluetoothDevice->controller()->createServiceObject(ledMatrinxServiceUuid, this);
        if (!m_ledMatrixService) {
            qCWarning(dcSenic()) << "Could not create led matrix service.";
            emit deviceInitializationFinished(false);
            return;
        }

        connect(m_ledMatrixService, &QLowEnergyService::stateChanged, this, &Nuimo::onLedMatrixServiceStateChanged);

        if (m_ledMatrixService->state() == QLowEnergyService::DiscoveryRequired) {
            m_ledMatrixService->discoverDetails();
        }
    }
    emit deviceInitializationFinished(true);
}

void Nuimo::onDeviceInfoServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcSenic()) << "Device info service discovered.";

    printService(m_deviceInfoService);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QString firmware = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::CharacteristicType::FirmwareRevisionString).value());
    QString hardware = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::CharacteristicType::HardwareRevisionString).value());
    QString software = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::CharacteristicType::SoftwareRevisionString).value());
#else
    QString firmware = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::FirmwareRevisionString).value());
    QString hardware = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::HardwareRevisionString).value());
    QString software = QString::fromUtf8(m_deviceInfoService->characteristic(QBluetoothUuid::SoftwareRevisionString).value());
#endif

    emit deviceInformationChanged(firmware, hardware, software);
}

void Nuimo::onBatteryServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcSenic()) << "Battery service discovered.";

    printService(m_batteryService);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_batteryCharacteristic = m_batteryService->characteristic(QBluetoothUuid::CharacteristicType::BatteryLevel);
#else
    m_batteryCharacteristic = m_batteryService->characteristic(QBluetoothUuid::BatteryLevel);
#endif
    if (!m_batteryCharacteristic.isValid()) {
        qCWarning(dcSenic()) << "Battery characteristc not found for thing " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }

    // Enable notifications
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QLowEnergyDescriptor notificationDescriptor = m_batteryCharacteristic.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
#else
    QLowEnergyDescriptor notificationDescriptor = m_batteryCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
#endif
    m_batteryService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));

    uint batteryPercentage = m_batteryCharacteristic.value().toHex().toUInt(nullptr, 16);
    emit batteryValueChanged(batteryPercentage);
}

void Nuimo::onBatteryCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == m_batteryCharacteristic.uuid()) {
        uint batteryPercentage = value.toHex().toUInt(nullptr, 16);
        emit batteryValueChanged(batteryPercentage);
    }
}

void Nuimo::onInputServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered
    if (state != QLowEnergyService::ServiceDiscovered)
        return;

    qCDebug(dcSenic()) << "Input service discovered.";

    printService(m_inputService);

    // Button
    m_inputButtonCharacteristic = m_inputService->characteristic(inputButtonCharacteristicUuid);
    if (!m_inputButtonCharacteristic.isValid()) {
        qCWarning(dcSenic()) << "Input button characteristc not found for thing " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }
    // Enable notifications
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QLowEnergyDescriptor notificationDescriptor = m_inputButtonCharacteristic.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
#else
    QLowEnergyDescriptor notificationDescriptor = m_inputButtonCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
#endif
    m_inputService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));


    // Swipe
    m_inputSwipeCharacteristic = m_inputService->characteristic(inputSwipeCharacteristicUuid);
    if (!m_inputSwipeCharacteristic.isValid()) {
        qCWarning(dcSenic()) << "Input swipe characteristc not found for thing " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }
    // Enable notifications
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    notificationDescriptor = m_inputSwipeCharacteristic.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
#else
    notificationDescriptor = m_inputSwipeCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
#endif
    m_inputService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));


    // Rotation
    m_inputRotationCharacteristic = m_inputService->characteristic(inputRotationCharacteristicUuid);
    if (!m_inputRotationCharacteristic.isValid()) {
        qCWarning(dcSenic()) << "Input rotation characteristc not found for thing " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }
    // Enable notifications
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    notificationDescriptor = m_inputRotationCharacteristic.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
#else
    notificationDescriptor = m_inputRotationCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
#endif
    m_inputService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));
}

void Nuimo::onInputCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    if (characteristic.uuid() == m_inputButtonCharacteristic.uuid()) {
        bool pressed = (bool)value.toHex().toUInt(nullptr, 16);
        qCDebug(dcSenic()) << "Button:" << (pressed ? "pressed": "released");
        if (pressed) {
            m_longPressTimer->start(m_longPressTime);
        } else {
            if (m_longPressTimer->isActive()) {
                m_longPressTimer->stop();
                emit buttonPressed();
            }
            // else the time run out and has the long pressed event emittted
        }
        return;
    }

    if (characteristic.uuid() == m_inputSwipeCharacteristic.uuid()) {
        quint8 swipe = (quint8)value.toHex().toUInt(nullptr, 16);
        switch (swipe) {
        case 0:
            qCDebug(dcSenic()) << "Swipe: Left";
            emit swipeDetected(SwipeDirectionLeft);
            break;
        case 1:
            qCDebug(dcSenic()) << "Swipe: Right";
            emit swipeDetected(SwipeDirectionRight);
            break;
        case 2:
            qCDebug(dcSenic()) << "Swipe: Up";
            emit swipeDetected(SwipeDirectionUp);
            break;
        case 3:
            qCDebug(dcSenic()) << "Swipe: Down";
            emit swipeDetected(SwipeDirectionDown);
            break;
        default:
            break;
        }
        return;
    }

    if (characteristic.uuid() == m_inputRotationCharacteristic.uuid()) {
        qint16 intValue = qFromLittleEndian<quint16>((uchar *)value.constData());
        qCDebug(dcSenic()) << "Rotation" << value.toHex() << intValue;
        int finalValue = m_rotationValue + qRound(intValue / 10.0);
        if (finalValue <= 0) {
            m_rotationValue = 0;
        } else if (finalValue >= 100) {
            m_rotationValue = 100;
        } else {
            m_rotationValue = finalValue;
        }
        emit rotationValueChanged(m_rotationValue);
        return;
    }

    qCDebug(dcSenic()) << "Service characteristic changed" << characteristic.name() << value.toHex();
}

void Nuimo::onLedMatrixServiceStateChanged(const QLowEnergyService::ServiceState &state)
{
    // Only continue if discovered

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (state != QLowEnergyService::RemoteServiceDiscovered)
        return;
#else
    if (state != QLowEnergyService::ServiceDiscovered)
        return;
#endif

    qCDebug(dcSenic()) << "Led matrix service discovered.";

    printService(m_ledMatrixService);

    // Led matrix
    m_ledMatrixCharacteristic = m_ledMatrixService->characteristic(ledMatrixCharacteristicUuid);
    if (!m_ledMatrixCharacteristic.isValid()) {
        qCWarning(dcSenic()) << "Led matrix characteristc not found for thing " << bluetoothDevice()->name() << bluetoothDevice()->address().toString();
        return;
    }
}
