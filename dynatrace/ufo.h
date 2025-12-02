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

#ifndef DYNATRACE_UFO_H
#define DYNATRACE_UFO_H

#include "network/networkaccessmanager.h"
#include "integrations/integrationplugin.h"

#include <QObject>
#include <QTimer>
#include <QColor>
#include <QHostAddress>

class Ufo : public QObject
{
    Q_OBJECT
public:
    explicit Ufo(NetworkAccessManager *networkManager, const QHostAddress &address, QObject *parent = nullptr);

    void getId();
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
    void idReceived(const QString &id);

};
#endif //DYNATRACE_UFO_H
