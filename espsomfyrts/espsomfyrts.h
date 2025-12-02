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

#ifndef ESPSOMFYRTS_H
#define ESPSOMFYRTS_H

#include <QUrl>
#include <QTimer>
#include <QObject>
#include <QWebSocket>

class NetworkDeviceMonitor;

class EspSomfyRts : public QObject
{
    Q_OBJECT
public:
    enum ShadeType {
        ShadeTypeRollerShade = 0,
        ShadeTypeBlind = 1,
        ShadeTypeDrapery = 2,
        ShadeTypeAwning = 3,
        ShadeTypeShutter = 4
    };
    Q_ENUM(ShadeType)

    enum MovingDirection {
        MovingDirectionUp = -1,
        MovingDirectionRest = 0,
        MovingDirectionDown = 1
    };
    Q_ENUM(MovingDirection)

    enum TileType {
        TileTypeNone = 0,
        TileTypeSeparateTileMotor = 1,
        TileTypeIntegratedTileMechanism = 2,
        TileTypeTileOnly = 3
    };
    Q_ENUM(TileType)

    enum SensorFlag {
        SensorFlagSunOn = 0x01,
        SensorFlagDemoMode = 0x04,
        SensorFlagWindy = 0x10,
        SensorFlagSuny = 0x20,
    };
    Q_DECLARE_FLAGS(SensorFlags, SensorFlag)
    Q_FLAG(SensorFlags)

    enum ShadeCommand {
        ShadeCommandMy,
        ShadeCommandUp,
        ShadeCommandDown,
        ShadeCommandMyUp,
        ShadeCommandMyDown,
        ShadeCommandUpDown,
        ShadeCommandMyUpDown,
        ShadeCommandProg,
        ShadeCommandSunFlag, // Turns the sun sensor on
        ShadeCommandFlag, // Turns the sun sensor off
        ShadeCommandStepUp,
        ShadeCommandStepDown,
        ShadeCommandFavorite,
        ShadeCommandStop
    };
    Q_ENUM(ShadeCommand)

    explicit EspSomfyRts(NetworkDeviceMonitor *monitor, QObject *parent = nullptr);

    NetworkDeviceMonitor *monitor() const;
    QHostAddress address() const;

    bool connected() const;
    uint signalStrength() const;
    QString firmwareVersion() const;


    QUrl shadesUrl();
    QUrl shadeCommandUrl();
    QUrl tiltCommandUrl();

    static QString getShadeCommandString(ShadeCommand shadeCommand);

signals:
    void connectedChanged(bool connected);
    void signalStrengthChanged(uint signalStrength);
    void firmwareVersionChanged(const QString &firmwareVersion);
    void shadeStateReceived(const QVariantMap &shadeState);

private slots:
    void onMonitorReachableChanged(bool reachable);
    void onWebSocketTextMessageReceived(const QString &message);

private:
    NetworkDeviceMonitor *m_monitor = nullptr;

    QUrl m_websocketUrl;
    QWebSocket *m_webSocket = nullptr;
    QTimer m_reconnectTimer;

    bool m_connected = false;
    uint m_signalStrength = 0;
    QString m_firmwareVersion;

    QUrl buildUrl(const QString &path);
};

#endif // ESPSOMFYRTS_H
