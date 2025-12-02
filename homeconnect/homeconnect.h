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

#ifndef HOMECONNECT_H
#define HOMECONNECT_H

#include <QObject>
#include <QTimer>
#include <QUuid>

#include <network/networkaccessmanager.h>

class HomeConnect : public QObject
{
    Q_OBJECT
public:
    enum EventType {
        EventTypeKeepAlive,
        EventTypeStatus,
        EventTypeEvent,
        EventTypeNotify,
        EventTypeDisconnected,
        EventTypeConnected,
        EventTypePaired,
        EventTypeDepaired
    };

    enum Type {
        Oven,
        Dishwasher,
        Washer,
        Dryer,
        WasherDryer,
        FridgeFreezer,
        Refrigerator,
        Freezer,
        WineCooler,
        CoffeeMaker,
        Hood,
        CleaningRobot,
        CookProcessor
    };

    struct HomeAppliance {
        QString name;
        QString brand;
        QString enumber;
        QString vib;
        bool connected;
        QString type;
        QString homeApplianceId;
    };

    struct OptionData {
        QString key;
        QVariant value;
        QString unit;
    };

    struct Option {
        QString key;
        QVariant value;
        QString unit;
    };

    /*
      "key": "Cooking.Oven.Option.SetpointTemperature",
      "name": "Target temperature for the oven",
      "uri": "/api/homeappliances/BOSCH-HNG6764B6-0000000011FF/programs/active/options/Cooking.Oven.Option.SetpointTemperature",
      "timestamp": 1556793979,
      "level": "hint",
      "handling": "none",
      "value": 200,
      "unit": "°C"
    */
    struct Event {
        QString key;
        QString name;
        QString uri;
        int timestamp;
        QVariant value;
        QString unit;
    };

    HomeConnect(NetworkAccessManager *networkmanager, const QByteArray &clientKey, const QByteArray &clientSecret, bool simulationMode = false, QObject *parent = nullptr);
    QByteArray accessToken();
    QByteArray refreshToken();
    void setSimulationMode(bool simulation);

    QUrl getLoginUrl(const QUrl &redirectUrl, const QString &scope);

    void getAccessTokenFromRefreshToken(const QByteArray &refreshToken);
    void getAccessTokenFromAuthorizationCode(const QByteArray &authorizationCode);

    // DEFAULT
    void getHomeAppliances(); // Get all home appliances which are paired with the logged-in user account.

    // PROGRAMS
    void getPrograms(const QString &haId); //Get all programs of a given home appliance
    void getProgramsAvailable(const QString &haId); //Get all programs which are currently available on the given home appliance
    void getProgramsActive(const QString &haId);    //Get program which is currently executed
    void getProgramsSelected(const QString &haId);  //Get the program which is currently selected
    void getProgramsActiveOption(const QString &haId, const QString &optionKey);
    QUuid selectProgram(const QString &haId, const QString &programKey, QList<Option> options);
    QUuid setSelectedProgramOptions(const QString &haId, QList<Option> options);
    QUuid startProgram(const QString &haId, const QString &programKey, QList<Option> options);
    QUuid stopProgram(const QString &haId);

    // STATUS EVENTS
    void getStatus(const QString &haid);
    void connectEventStream();

    // SETTINGS
    void getSettings(const QString &haid);

    // COMMANDS
    QUuid sendCommand(const QString &haid, const QString &command); //commands "BSH.Common.Command.ResumeProgram" & "PauseProgram"

private:
    bool m_simulationMode = false;
    QByteArray m_baseAuthorizationUrl;
    QByteArray m_baseTokenUrl;
    QByteArray m_baseControlUrl;
    QByteArray m_clientKey;
    QByteArray m_clientSecret;

    QByteArray m_accessToken;
    QByteArray m_refreshToken;
    QByteArray m_redirectUri  = "https://127.0.0.1:8888";
    QString m_codeChallenge;

    NetworkAccessManager *m_networkManager = nullptr;
    QTimer *m_tokenRefreshTimer = nullptr;

    void setAuthenticated(bool state);
    void setConnected(bool state);

    bool m_authenticated = false;
    bool m_connected = false;

    bool checkStatusCode(QNetworkReply *reply, const QByteArray &rawData);

private slots:
    void onRefreshTimeout();

signals:
    void connectionChanged(bool connected);
    void authenticationStatusChanged(bool authenticated);
    void receivedRefreshToken(const QByteArray &refreshToken);
    void receivedAccessToken(const QByteArray &accessToken);
    void commandExecuted(const QUuid &commandId, bool success);

    void receivedHomeAppliances(const QList<HomeConnect::HomeAppliance> &appliances);
    void receivedStatusList(const QString &haId, const QHash<QString, QVariant> &statusList);
    void receivedPrograms(const QString &haId, const QStringList &programs);
    void receivedAvailablePrograms(const QString &haId, const QStringList &programs);
    void receivedSettings(const QString &haId, const QHash<QString, QVariant> &settings);
    void receivedActiveProgram(const QString &haId, const QString &key, const QHash<QString, QVariant> &options);
    void receivedSelectedProgram(const QString &haId, const QString &key, const QHash<QString, QVariant> &options);
    void receivedEvents(HomeConnect::EventType eventType, const QString &haId, const QList<HomeConnect::Event> &events);
};
#endif // HOMECONNECT_H
