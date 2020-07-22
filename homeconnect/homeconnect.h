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

#ifndef HOMECONNECT_H
#define HOMECONNECT_H

#include <QObject>
#include <QTimer>
#include <QUuid>

#include "network/networkaccessmanager.h"

class HomeConnect : public QObject
{
    Q_OBJECT
public:
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
    void getHomeAppliance(const QString &haid); //Get a specfic home appliances which are paired with the logged-in user account.

    // PROGRAMS
    void getProgramsAvailable(const QString &haId); //Get all programs which are currently available on the given home appliance
    void getProgramsActive(const QString &haId);    //Get program which is currently executed
    void getProgramsSelected(const QString &haId);  //Get the program which is currently selected
    void getProgramsActiveOption(const QString &haId, const QString &optionKey);
    QUuid startProgram(const QString &haId, const QString &programKey, QList<Option> options);

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

    bool checkStatusCode(int status, const QByteArray &payload);
private slots:
    void onRefreshTimeout();

signals:
    void connectionChanged(bool connected);
    void authenticationStatusChanged(bool authenticated);
    void commandExecuted(QUuid commandId,bool success);

    void receivedHomeAppliances(const QList<HomeAppliance> &appliances);
    void receivedStatusList(const QString &haId, const QHash<QString, QVariant> &statusList);
    void receivedAvailablePrograms(const QString &haId, const QStringList &programs);
    void receivedSettings(const QString &haId, const QHash<QString, QVariant> &settings);
    void receivedActiveProgram(const QString &haId, const QString &key, const QHash<QString, QVariant> &options);
    void receivedSelectedProgram(const QString &haId, const QString &key, const QHash<QString, QVariant> &options);
    void receivedEvents(const QList<Event> &events);
};
#endif // HOMECONNECT_H
