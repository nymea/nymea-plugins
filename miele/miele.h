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

#ifndef MIELE_H
#define MIELE_H

#include <QObject>
#include <QTimer>
#include <QUuid>
#include <QJsonDocument>

#include "network/networkaccessmanager.h"

class Miele : public QObject
{
    Q_OBJECT
public:
    enum ProcessAction {
        ProcessActionsStart = 0,
        ProcessActionsStop,
        ProcessActionsPause,
        ProcessActionsStarSuperfreezing,
        ProcessActionsStopSuperfreezing,
        ProcessActionsStartSupercooling,
        ProcessActionsStopSupercooling
    };

    enum Mode {
        ModeNormal,
        ModeSabbath
    };

    enum Color {
        ColorWhite,
        ColorBlue,
        ColorRed,
        ColorYellow,
        ColorOrange,
        ColorGreen,
        ColorPink,
        ColorPurple,
        ColorTurquoise
    };
    Q_ENUM(Color)

    enum Status {
        StatusOff = 1,
        StatusOn = 2,
        StatusProgrammed = 3,
        StatusProgrammedWaitingToStart = 4,
        StatusRunnin = 5,
        StatusPause = 6,
        StatusEndProgrammed = 7,
        StatusFailure = 8,
        StatusProgrammInterrupted = 9,
        StatusIdle = 10,
        StatusRinseHold = 11,
        StatusService = 12,
        StatusSuperfreezing = 13,
        StatusSupercooling = 14,
        StatusSuperheating = 15,
        StatusSupercoolingSuperfreezing = 146,
        StatusNotConnected = 255
    };

    struct DeviceShort {
        QString details;
        QString name;
        QString fabNumber;
        QString state;
        QString type;
    };

    Miele(NetworkAccessManager *networkmanager, const QByteArray &clientId, const QByteArray &clientSecret, const QString &language = "en", QObject *parent = nullptr);
    QByteArray accessToken();
    QByteArray refreshToken();

    QUrl getLoginUrl(const QUrl &redirectUrl, const QString &state = "");

    void getAccessTokenFromRefreshToken(const QByteArray &refreshToken);
    void getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode);

    // INFORMATION
    void getDevices();
    void getDevicesShort();
    void getDevice(const QString &deviceId);
    void getDeviceState(const QString &deviceId);

    // ACTION
    void getActions(const QString &deviceId); //The GET action is used to request a device to send the currently supported actions
    QUuid processAction(const QString &deviceId, ProcessAction action);

    QUuid setPower(const QString &deviceId, bool power);
    QUuid setDeviceName(const QString &deviceId, const QString &deviceName);
    QUuid setLight(const QString &deviceId, bool power);
    QUuid setTargetTemperature(const QString &deviceId, int zone, int targetTemperature);
    QUuid setColors(const QString &deviceId, Color color);
    QUuid setModes(const QString &deviceId, Mode mode);
    QUuid setVentilationStep(const QString &deviceId, int step);
    QUuid setStartTime(const QString &deviceId, int seconds);

    // EVENTS
    void getAllEvents();
private:
    QString m_language;
    QByteArray m_clientId;
    QByteArray m_clientSecret;
    NetworkAccessManager *m_networkManager = nullptr;

    QUuid putAction(const QString &deviceId, const QJsonDocument &action); //

    QUrl m_authorizationUrl = QUrl("https://api.mcs3.miele.com/thirdparty/login/");
    QUrl m_tokenUrl = QUrl("https://api.mcs3.miele.com/thirdparty/token");
    QUrl m_apiUrl = QUrl("https://api.mcs3.miele.com");

    QByteArray m_accessToken;
    QByteArray m_refreshToken;
    QByteArray m_redirectUri  = "https://127.0.0.1:8888";

    QTimer *m_tokenRefreshTimer = nullptr;
    int m_refreshInterval = (60 * 10); //10 minutes
    qint64 m_accessTokenExpireTime = 0;

    bool m_authenticated = false;
    bool m_connected = false;

    void setAuthenticated(bool state);
    void setConnected(bool state);
    bool checkStatusCode(QNetworkReply *reply, const QByteArray &rawData);
signals:
    void connectionChanged(bool connected);
    void authenticationStatusChanged(bool authenticated);
    void receivedRefreshToken(const QByteArray &refreshToken);
    void receivedAccessToken(const QByteArray &accessToken);
    void commandExecuted(const QUuid &commandId, bool success);
    void devicesFound(QList<DeviceShort> devices);
    void deviceStateReceived(const QString &deviceId, const QVariantMap &deviceState);

private slots:
    void onRefreshTimeout();
};
#endif // MIELE
