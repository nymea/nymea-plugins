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
