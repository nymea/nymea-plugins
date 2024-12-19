/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2024, nymea GmbH
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
