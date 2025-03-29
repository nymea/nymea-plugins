/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#ifndef TADO_H
#define TADO_H

#include <network/networkaccessmanager.h>
#include <integrations/thing.h>

#include <QObject>
#include <QTimer>
#include <QUuid>

class Tado : public QObject
{
    Q_OBJECT
public:
    struct Zone {
        QString id;
        QString name;
        QString type;
    };

    struct Overlay {
      bool power;
      double temperature;
      QString zoneType;
      QString terminationType;
      QString tadoMode;
    };

    struct ZoneState {
        bool connected = false;
        bool power = false;
        QString tadoMode;
        QString settingType;
        double settingTemperature = false;
        bool settingPower = false;
        double temperature = false;
        double humidity = false;
        bool windowOpenDetected = false;
        double heatingPowerPercentage = false;
        QString heatingPowerType;
        bool overlayIsSet = false;
        bool overlaySettingPower = false;
        double overlaySettingTemperature = false;
        QString overlayType;
    };

    struct Home {
        QString id;
        QString name;
    };

    explicit Tado(NetworkAccessManager *networkManager, QObject *parent = nullptr);

    bool apiAvailable();
    bool authenticated();
    bool connected();

    QString loginUrl() const;

    QString username() const;

    QString refreshToken() const;
    void setRefreshToken(const QString &refreshToken);

    // Login process
    void getLoginUrl();
    void startAuthentication();
    void getAccessToken();

    void getHomes();
    void getZones(const QString &homeId);
    void getZoneState(const QString &homeId, const QString &zoneId);

    QUuid setOverlay(const QString &homeId, const QString &zoneId, bool power, double targetTemperature);
    QUuid deleteOverlay(const QString &homeId, const QString &zoneId);

private:
    bool m_apiAvailable = false;
    QString m_baseAuthorizationUrl;
    QString m_baseControlUrl;
    QString m_clientId;
    QString m_deviceCode;

    NetworkAccessManager *m_networkManager = nullptr;
    QString m_loginUrl;
    QString m_accessToken;
    QString m_refreshToken;
    QString m_username;

    QTimer m_refreshTimer;
    QTimer m_pollAuthenticationTimer;
    uint m_pollAuthenticationCount = 0;
    uint m_pollAuthenticationLimit = 5;

    bool m_authenticationStatus = false;
    bool m_connectionStatus = false;

    void requestAuthenticationToken();

    void setAuthenticationStatus(bool status);
    void setConnectionStatus(bool status);

signals:
    void connectionChanged(bool connected);
    void getLoginUrlFinished(bool success);
    void startAuthenticationFinished(bool success);
    void accessTokenReceived();
    void usernameChanged(const QString &username);
    void refreshTokenReceived(const QString &refreshToken);

    void authenticationStatusChanged(bool authenticated);

    void requestExecuted(QUuid requestId, bool success);

    void homesReceived(QList<Tado::Home> homes);
    void zonesReceived(const QString &homeId, QList<Tado::Zone> zones);
    void zoneStateReceived(const QString &homeId,const QString &zoneId, Tado::ZoneState sate);
    void overlayReceived(const QString &homeId, const QString &zoneId, const Tado::Overlay &overlay);
    void connectionError(QNetworkReply::NetworkError error);

};

#endif // TADO_H
