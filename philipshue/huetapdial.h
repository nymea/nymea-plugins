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

#ifndef HUETAPDIAL_H
#define HUETAPDIAL_H

#include <QObject>
#include <QTimer>

#include "extern-plugininfo.h"
#include "huedevice.h"

class HueTapDial : public HueDevice
{
    Q_OBJECT
public:
    explicit HueTapDial(HueBridge *bridge, QObject *parent = nullptr);
    //virtual ~HueTapDial() = default;

    int rotaryId() const;
    void setRotaryId(int sensorId);

    QString rotaryUuid() const;
    void setRotaryUuid(const QString &rotaryUuid);

    int switchId() const;
    void setSwitchId(int sensorId);

    QString switchUuid() const;
    void setSwitchUuid(const QString &switchUuid);

    int level() const;
    int batteryLevel() const;

    void updateStates(const QVariantMap &sensorMap);

    bool isValid();
    bool hasSensor(int sensorId);
    bool hasSensor(const QString &sensorUuid);

private:
    // Params
    int m_rotaryId;
    QString m_rotaryUuid;

    int m_switchId;
    QString m_switchUuid;

    // States
    QString m_lastUpdateButton;
    QString m_lastUpdateRotation;
    double m_level = 0;
    int m_batteryLevel = 0;
    int m_lastButtonCode = -1;
    int m_lastRotationCode = 0;

signals:
    void levelChanged(double level);
    void batteryLevelChanged(int batteryLevel);
    void buttonPressed(int buttonCode);
    void rotated(int rotationCode);

};

#endif // HUETAPDIAL_H
