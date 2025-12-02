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

#ifndef LIFXCLOUD_H
#define LIFXCLOUD_H

#include <QObject>
#include <QColor>
#include <QTimer>

#include <network/networkaccessmanager.h>

class LifxCloud : public QObject
{
    Q_OBJECT
public:
    enum State {
        StatePower,
        StateBrightness,
        StateColor,
        StateColorTemperature,
        StateInfrared
    };

    enum Effect {
        EffectNone,
        EffectBreathe,
        EffectMove,
        EffectMorph,
        EffectFlame,
        EffectPulse
    };

    struct Group {
        QByteArray id;
        QString name;
    };

    struct Location {
        QByteArray id;
        QString name;
    };

    struct Scene {
        QByteArray id;
        QString name;
    };

    struct Capabilities {
        bool color;
        bool colorTemperature;
        bool ir;
        bool chain;
        bool multizone;
        int minKelvin;
        int maxKelvin;
    };

    struct Product {
        QString name;
        QString identifier;
        QString manufacturer;
        uint secondsSinceLastSeen;
        Capabilities capabilities;
    };

    struct Light {
        QByteArray id;
        QByteArray uuid;
        QString label;
        bool connected;
        bool power;
        QColor color;
        int colorTemperature;
        double brightness;
        Group group;
        Location location;
        Product product;
    };

    explicit LifxCloud(NetworkAccessManager *networkManager, QObject *parent = nullptr);

    void setAuthorizationToken(const QByteArray &token);
    bool cloudAuthenticated();
    bool cloudConnected();

    void listLights();
    void listScenes();
    int setPower(const QString &lightId, bool power, int duration = 0);
    int setBrightnesss(const QString &lightId, int brightness, int duration = 0);
    int setColor(const QString &lightId, QColor color, int duration = 0);
    int setColorTemperature(const QString &lightId, int kelvin, int duration = 0);
    int setInfrared(const QString &lightId, int infrared, int duration = 0);

    int activateScene(const QString &sceneId);

    int setEffect(const QString &lightId, Effect effect, QColor color = QColor("#FFFFFF"));

private:
    NetworkAccessManager *m_networkManager = nullptr;
    QByteArray m_authorizationToken;

    int setState(const QString &lightId, State state, QVariant stateValue, int duration);
    bool checkHttpStatusCode(QNetworkReply *reply);
    bool m_authenticated = false;
    bool m_connected = false;

signals:
    void connectionChanged(bool m_connected);
    void authenticationChanged(bool m_authenticated);
    void lightsListReceived(const QList<LifxCloud::Light> &lights);
    void scenesListReceived(const QList<LifxCloud::Scene> &scenes);
    void requestExecuted(int requestId, bool susccess);
};

#endif // LIFXCLOUD_H
