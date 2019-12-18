/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TADO_H
#define TADO_H

#include "network/networkaccessmanager.h"
#include "devices/device.h"

#include <QObject>
#include <QTimer>

class Tado : public QObject
{
    Q_OBJECT
public:
    struct Token {
        QString accesToken;
        QString tokenType;
        QString refreshToken;
        int expires;
        QString scope;
        QString jti;
    };

    struct Zone {
        QString id;
        QString name;
        QString type;
    };

    struct ZoneState {
        bool connected;
        bool power;
        QString tadoMode;
        QString settingType;
        double settingTemperature;
        bool settingPower;
        double temperature;
        double humidity;
        bool windowOpen;
        double heatingPowerPercentage;
        QString heatingPowerType;
        bool overlayIsSet;
        bool overlaySettingPower;
        double overlaySettingTemperature;
        QString overlayType;
    };

    struct Home {
        QString id;
        QString name;
    };

    explicit Tado(NetworkAccessManager *networkManager, const QString &username, QObject *parent = nullptr);

    void setUsername(const QString &username);
    QString username();

    void getToken(const QString &password);
    void getHomes();
    void getZones(const QString &homeId);
    void getZoneState(const QString &homeId, const QString &zoneId);

    void setOverlay(const QString &homeId, const QString &zoneId, const QString &mode, double targetTemperature);
    void deleteOverlay(const QString &homeId, const QString &zoneId);

private:
    QByteArray m_baseAuthorizationUrl = "https://auth.tado.com/oauth/token";
    QByteArray m_baseControlUrl = "https://my.tado.com/api/v2";
    QByteArray m_clientSecret = "4HJGRffVR8xb3XdEUQpjgZ1VplJi6Xgw";
    QByteArray m_clientId = "public-api-preview";

    NetworkAccessManager *m_networkManager = nullptr;
    QString m_username;
    QString m_accessToken;
    QString m_refreshToken;
    QTimer *m_refreshTimer = nullptr;

signals:
    void connectionChanged(bool connected);
    void authenticationStatusChanged(bool authenticated);

    void tokenReceived(Token token);

    void homesReceived(QList<Home> homes);
    void zonesReceived(const QString &homeId, QList<Zone> zones);
    void zoneStateReceived(const QString &homeId,const QString &zoneId, ZoneState sate);
private slots:
    void onRefreshTimer();

};

#endif // TADO_H
