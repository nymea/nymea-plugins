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

#include "espsomfyrts.h"
#include "extern-plugininfo.h"

#include <network/networkdevicemonitor.h>

#include <QJsonDocument>
#include <QJsonParseError>

EspSomfyRts::EspSomfyRts(NetworkDeviceMonitor *monitor, QObject *parent)
    : QObject{parent},
    m_monitor{monitor}
{
    m_websocketUrl.setScheme("ws");
    m_websocketUrl.setHost("127.0.0.1");
    m_websocketUrl.setPort(8080);

    m_webSocket = new QWebSocket("nymea", QWebSocketProtocol::Version13, this);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &EspSomfyRts::onWebSocketTextMessageReceived);
    connect(m_webSocket, &QWebSocket::connected, this, [this](){
        qCDebug(dcESPSomfyRTS()) << "Websocket connected";
        m_connected = true;
        emit connectedChanged(m_connected);
    });

    connect(m_webSocket, &QWebSocket::disconnected, this, [this](){
        qCDebug(dcESPSomfyRTS()) << "Websocket disconnected";
        m_connected = false;
        emit connectedChanged(m_connected);

        m_reconnectTimer.start();
    });

    if (m_monitor) {
        qCDebug(dcESPSomfyRTS()) << "Setting up ESP Somfy using the network device monitor on" << m_monitor->macAddress();
        connect(m_monitor, &NetworkDeviceMonitor::reachableChanged, this, &EspSomfyRts::onMonitorReachableChanged);

        // Init connection based on the monitor
        onMonitorReachableChanged(m_monitor->reachable());
    }

    // Websocket reconnect mechanism
    m_reconnectTimer.setInterval(5);
    m_reconnectTimer.setSingleShot(false);
    connect(&m_reconnectTimer, &QTimer::timeout, this, [this](){
        if (m_webSocket->state() == QAbstractSocket::UnconnectedState && m_monitor->reachable()) {
            m_websocketUrl.setHost(m_monitor->networkDeviceInfo().address().toString());
            qCDebug(dcESPSomfyRTS()) << "Trying to connect to" << m_websocketUrl;
            m_webSocket->open(m_websocketUrl);
        }
    });
}

NetworkDeviceMonitor *EspSomfyRts::monitor() const
{
    return m_monitor;
}

QHostAddress EspSomfyRts::address() const
{
    return QHostAddress(m_websocketUrl.host());
}

bool EspSomfyRts::connected() const
{
    return m_connected;
}

QString EspSomfyRts::firmwareVersion() const
{
    return m_firmwareVersion;
}

QUrl EspSomfyRts::shadesUrl()
{
    return buildUrl("shades");
}

QUrl EspSomfyRts::shadeCommandUrl()
{
    return buildUrl("shadeCommand");
}

QUrl EspSomfyRts::tiltCommandUrl()
{
    return buildUrl("tiltCommand");
}

QString EspSomfyRts::getShadeCommandString(ShadeCommand shadeCommand)
{
    QString shadeCommandString;

    switch(shadeCommand) {
    case ShadeCommandMy:
        shadeCommandString = "m";
        break;
    case ShadeCommandUp:
        shadeCommandString = "u";
        break;
    case ShadeCommandDown:
        shadeCommandString = "d";
        break;
    case ShadeCommandMyUp:
        shadeCommandString = "mu";
        break;
    case ShadeCommandMyDown:
        shadeCommandString = "md";
        break;
    case ShadeCommandUpDown:
        shadeCommandString = "ud";
        break;
    case ShadeCommandMyUpDown:
        shadeCommandString = "mud";
        break;
    case ShadeCommandProg:
        shadeCommandString = "p";
        break;
    case ShadeCommandSunFlag:
        shadeCommandString = "s";
        break;
    case ShadeCommandFlag:
        shadeCommandString = "f";
        break;
    case ShadeCommandStepUp:
        shadeCommandString = "su";
        break;
    case ShadeCommandStepDown:
        shadeCommandString = "sd";
        break;
    case ShadeCommandFavorite:
        shadeCommandString = "fav";
        break;
    case ShadeCommandStop:
        shadeCommandString = "stop";
        break;
    }

    return shadeCommandString;
}

void EspSomfyRts::onMonitorReachableChanged(bool reachable)
{
    qCDebug(dcESPSomfyRTS()) << "Network device of" << m_websocketUrl.host() << "is" << (reachable ? "now reachable" : "not reachable any more");

    if (reachable) {
        if (m_webSocket->state() == QAbstractSocket::ConnectedState)
            return;

        m_websocketUrl.setHost(m_monitor->networkDeviceInfo().address().toString());
        qCDebug(dcESPSomfyRTS()) << "Connecting to" << m_websocketUrl.toString();
        m_webSocket->open(m_websocketUrl);
    }
}

void EspSomfyRts::onWebSocketTextMessageReceived(const QString &message)
{
    //qCDebug(dcESPSomfyRTS()) << "Websocket message received:" << message;

    if (message.startsWith("42")) {
        QJsonParseError jsonError;
        QByteArray rawMessage = message.mid(3, message.size() - 4).toUtf8();
        // Make parsing easier
        int index = rawMessage.indexOf(',');
        if (index < 0) {
            qCWarning(dcESPSomfyRTS()) << "Could not parse notification from data" << rawMessage;
            return;
        }

        QString notification = rawMessage.left(index);
        QByteArray rawPayload = rawMessage.right(rawMessage.size() - index - 1);

        QJsonDocument jsonDoc = QJsonDocument::fromJson(rawPayload, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            qCWarning(dcESPSomfyRTS()) << "Json error parsing the data" << rawPayload << jsonError.error << jsonError.errorString();
            return;
        }

        QVariantMap payload = jsonDoc.toVariant().toMap();

        if (notification == "wifiStrength") {

            uint signalStrength = 0;
            int dbm = payload.value("strength").toInt();
            if (dbm > -90)
                signalStrength += 20;
            if (dbm > -80)
                signalStrength += 20;
            if (dbm > -70)
                signalStrength += 20;
            if (dbm > -67)
                signalStrength += 20;
            if (dbm > -30)
                signalStrength += 20;

            if (m_signalStrength != signalStrength) {
                m_signalStrength = signalStrength;
                emit signalStrengthChanged(m_signalStrength);
            }

        } else if (notification == "fwStatus") {

            QString firmwareVersion = payload.value("fwVersion").toMap().value("name").toString();
            if (m_firmwareVersion != firmwareVersion) {
                m_firmwareVersion = firmwareVersion;
                emit firmwareVersionChanged(m_firmwareVersion);
            }

            // TODO. firmware update

        } else if (notification == "shadeState") {
            emit shadeStateReceived(payload);
        } else if (notification == "memStatus") {
            // We are not interested in this, filter it out
        } else {
            qCDebug(dcESPSomfyRTS()) << "Notification" << notification << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
        }

    }
}

QUrl EspSomfyRts::buildUrl(const QString &path)
{
    return QUrl(QString("http://%1/%2").arg(m_websocketUrl.host()).arg(path));
}

