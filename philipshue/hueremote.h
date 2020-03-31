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

#ifndef HUEREMOTE_H
#define HUEREMOTE_H

#include <QObject>
#include <QDebug>
#include <QHostAddress>
#include <QNetworkRequest>
#include <QJsonDocument>

#include "typeutils.h"
#include "huedevice.h"

class HueRemote : public HueDevice
{
    Q_OBJECT
public:
    enum ButtonCode {
        SmartButtonPressed = 1000,
        OnLongPressed = 1001,
        OnPressed = 1002,
        SmartButtonLongPressed = 1003,
        DimUpLongPressed = 2001,
        DimUpPressed = 2002,
        DimDownLongPressed = 3001,
        DimDownPressed = 3002,
        OffLongPressed = 4001,
        OffPressed = 4002,
        TapButton1Pressed = 34,
        TapButton2Pressed = 16,
        TapButton3Pressed = 17,
        TapButton4Pressed = 18
    };

    explicit HueRemote(QObject *parent = nullptr);

    int battery() const;
    void setBattery(const int &battery);

    void updateStates(const QVariantMap &statesMap, const QVariantMap &configMap);

private:
    int m_battery;
    QString m_lastUpdate;

signals:
    void stateChanged();
    void buttonPressed(const int &buttonCode);

};

#endif // HUEREMOTE_H
