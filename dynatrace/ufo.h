/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by copyright law, and
* remains the property of nymea GmbH. All rights, including reproduction, publication,
* editing and translation, are reserved. The use of this project is subject to the terms of a
* license agreement to be concluded with nymea GmbH in accordance with the terms
* of use of nymea GmbH, available under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* This project may also contain libraries licensed under the open source software license GNU GPL v.3.
* Alternatively, this project may be redistributed and/or modified under the terms of the GNU
* Lesser General Public License as published by the Free Software Foundation; version 3.
* this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along with this project.
* If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under contact@nymea.io
* or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef DYNATRACE_UFO_H
#define DYNATRACE_UFO_H

#include "network/networkaccessmanager.h"
#include "devices/device.h"

#include <QObject>
#include <QTimer>
#include <QColor>
#include <QHostAddress>

class Ufo : public QObject
{
    Q_OBJECT
public:
    explicit Ufo(NetworkAccessManager *networkManager, const QHostAddress &address, QObject *parent = nullptr);

    void resetLogo();
    void setLogo(QColor led1, QColor led2, QColor led3, QColor led4);
    void initBackgroundColor(bool top, bool bottom);
    void setBackgroundColor(bool top, bool initTop, bool bottom, bool initBottom, QColor color); //top and bottom flags are to select the ring, init is to reset a effect
    void setLeds(bool top, int ledIndex, int numOfLeds, QColor color);
    void startWhirl(bool top, bool bottom, QColor color, int speed, bool clockwise); //Speed: 0 (no movement) to about 510 (very fast)
    void stopWhirl(bool top, bool bottom);
    void startMorph(bool top, bool bottom, QColor color, int time, int speed); //time in ms, speed 0-10
    void stopMorph(bool top, bool bottom);


private:
    NetworkAccessManager *m_networkManager;
    QHostAddress m_address;

signals:
    void connectionChanged(bool reachable);

};
#endif //DYNATRACE_UFO_H
