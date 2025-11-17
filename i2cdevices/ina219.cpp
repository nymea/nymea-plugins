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

#include "ina219.h"

#include <unistd.h>
#include <QtDebug>
#include <QThread>
#include <QDebug>
#include <QJsonDocument>

#include "extern-plugininfo.h"

#define INA219_REGISTER_CONFIGURATION 0x00
#define INA219_REGISTER_SHUNT_VOLTAGE 0x01
#define INA219_REGISTER_BUS_VOLTAGE   0x02
#define INA219_REGISTER_POWER         0x03
#define INA219_REGISTER_CURRENT       0x04
#define INA219_REGISTER_CALIBRATION   0x05

#define INA219_ADDRESS 0x80
#define INA219_READ    0x01

#define SHUNT_MILLIVOLTS_LSB 0.01 //# 10uV
#define BUS_MILLIVOLTS_LSB 4 // 4mV
#define CALIBRATION_FACTOR 0.04096
#define MAX_CALIBRATION_VALUE 0xFFFE // Max value supported (65534 decimal)
//   # In the spec (p17) the current LSB factor for the minimum LSB is
//   # documented as 32767, but a larger value (100.1% of 32767) is used
//   # to guarantee that current overflow can always be detected.
#define CURRENT_LSB_FACTOR 32800

#define OVERFLOW_VALUE 1

#define INA219_CONFIG_BIT_RST   15
#define INA219_CONFIG_BIT_BRNG  13
#define INA219_CONFIG_BIT_PG1   12
#define INA219_CONFIG_BIT_PG0   11
#define INA219_CONFIG_BIT_BADC4 10
#define INA219_CONFIG_BIT_BADC3  9
#define INA219_CONFIG_BIT_BADC2  8
#define INA219_CONFIG_BIT_BADC1  7
#define INA219_CONFIG_BIT_SADC4  6
#define INA219_CONFIG_BIT_SADC3  5
#define INA219_CONFIG_BIT_SADC2  4
#define INA219_CONFIG_BIT_SADC1  3
#define INA219_CONFIG_BIT_MODE3  2
#define INA219_CONFIG_BIT_MODE2  1
#define INA219_CONFIG_BIT_MODE1  0


Ina219::Ina219(const QString &portName, int address, double shuntOhms, VoltageRange voltageRange, QObject *parent):
    I2CDevice(portName, address, parent),
    m_shuntOhms(shuntOhms),
    m_voltageRange(voltageRange)
{

}

bool Ina219::writeData(int fileDescriptor, const QByteArray &data)
{
    Q_UNUSED(data)

    char buf[3] = {0};

    // Calibration
    double gain;
    switch (m_gainVolts) {
    case GainVolts032:
        gain = 0.32;
        break;
    case GainVolts016:
        gain = 0.16;
        break;
    case GainVolts008:
        gain = 0.08;
        break;
    case GainVolts004:
    default:
        gain = 0.04;
        break;
    }

    double maxPossibleAmps = gain / m_shuntOhms;
    m_currentLSB = maxPossibleAmps / CURRENT_LSB_FACTOR;

    quint16 calibration = CALIBRATION_FACTOR / (m_currentLSB * m_shuntOhms);
    buf[0] = INA219_REGISTER_CALIBRATION;
    buf[1] = calibration >> 8;
    buf[2] = calibration & 0xFF;
    qCDebug(dcI2cDevices()) << "INA219 writing calibration:" << QString::number(calibration, 16) << QByteArray(buf, 3).toHex();
    int ret = write(fileDescriptor, buf, 3);
    if (ret != 3) {
        qCWarning(dcI2cDevices()) << "Failed to write calibration to INA219.";
        return false;
    }

    // Configuration
    quint16 configuration = m_voltageRange << INA219_CONFIG_BIT_BRNG;
    configuration |= m_gainVolts << INA219_CONFIG_BIT_PG0;
    configuration |= m_busADC << INA219_CONFIG_BIT_BADC1;
    configuration |= m_shuntADC << INA219_CONFIG_BIT_SADC1;
    configuration |= m_operationMode;
    buf[0] = INA219_REGISTER_CONFIGURATION;
    buf[1] = configuration >> 8;
    buf[2] = configuration & 0xFF;
    qCDebug(dcI2cDevices()) << "INA219 writing configuration:" << QString::number(configuration, 16) << QByteArray(buf, 3).toHex();
    ret = write(fileDescriptor, buf, 3);
    if (ret != 3) {
        qCWarning(dcI2cDevices()) << "Failed to write configuration to INA219.";
        return false;
    }

    return true;
}

QByteArray Ina219::readData(int fileDescriptor)
{
    char buf[2] = {0};

    buf[0] = INA219_REGISTER_SHUNT_VOLTAGE;
    int ret = write(fileDescriptor, buf, 1);
    if (ret != 1) {
        qCWarning(dcI2cDevices()) << "Failed to select shunt voltage register on INA219";
        return QByteArray();
    }
    ret = read(fileDescriptor, buf, 2);
    if (ret != 2) {
        qCWarning(dcI2cDevices()) << "Failed to read shunt voltage register on INA219";
        return QByteArray();
    }
    int shuntVoltageRaw = ((buf[0] << 8) | buf[1]);
    double shuntVoltage = shuntVoltageRaw * SHUNT_MILLIVOLTS_LSB / 1000;


    buf[0] = INA219_REGISTER_BUS_VOLTAGE;
    ret = write(fileDescriptor, buf, 1);
    if (ret != 1) {
        qCWarning(dcI2cDevices()) << "Failed to select bus voltage register on INA219";
        return QByteArray();
    }
    ret = read(fileDescriptor, buf, 2);
    if (ret != 2) {
        qCWarning(dcI2cDevices()) << "Failed to read bus voltage register on INA219";
        return QByteArray();
    }
    int busVoltageRaw = ((buf[0] << 8) | buf[1]);
    bool overflow = (busVoltageRaw & OVERFLOW_VALUE) == 1;
    busVoltageRaw = busVoltageRaw >> 3; // Registers are not right_aligned
    double busVoltage = 1.0 * busVoltageRaw * BUS_MILLIVOLTS_LSB / 1000;

    buf[0] = INA219_REGISTER_POWER;
    ret = write(fileDescriptor, buf, 1);
    if (ret != 1) {
        qCWarning(dcI2cDevices()) << "Failed to select power register on INA219";
        return QByteArray();
    }
    ret = read(fileDescriptor, buf, 2);
    if (ret != 2) {
        qCWarning(dcI2cDevices()) << "Failed to read power register on INA219";
        return QByteArray();
    }
    int powerRaw = ((buf[0] << 8) | buf[1]);
    double powerLSB = m_currentLSB * 20;
    double power = powerRaw * powerLSB;

    buf[0] = INA219_REGISTER_CURRENT;
    ret = write(fileDescriptor, buf, 1);
    if (ret != 1) {
        qCWarning(dcI2cDevices()) << "Failed to select current register on INA219";
        return QByteArray();
    }
    ret = read(fileDescriptor, buf, 2);
    if (ret != 2) {
        qCWarning(dcI2cDevices()) << "Failed to read current register on INA219";
        return QByteArray();
    }
    int currentRaw = ((buf[0] << 8) | buf[1]);
    double current = 1.0 * currentRaw * m_currentLSB;

    qCDebug(dcI2cDevices()).nospace().noquote() << "INA219 Shunt voltage: " << shuntVoltage << "mV, Bus voltage: " << busVoltage << "V, Power: " << power << "W, Current: " << current << "A, Overflow: " << overflow;

    QVariantMap readings;
    readings.insert("shuntVoltage", shuntVoltage);
    readings.insert("busVoltage", busVoltage);
    readings.insert("power", power);
    readings.insert("current", current);
    readings.insert("overflow", overflow);
    return QJsonDocument::fromVariant(readings).toJson(QJsonDocument::Compact);
}
