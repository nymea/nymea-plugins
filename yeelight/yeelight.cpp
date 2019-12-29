/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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


#include "yeelight.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QColor>
#include <QRandomGenerator>

Yeelight::Yeelight(NetworkAccessManager *networkManager, const QHostAddress &address, quint16 port, QObject *parent) :
    QObject(parent),
    m_address(address),
    m_port(port),
    m_networkManager(networkManager)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::stateChanged, this, &Yeelight::onStateChanged);
    connect(m_socket, &QTcpSocket::readyRead, this, &Yeelight::onReadyRead);
    m_socket->connectToHost(address, port);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &Yeelight::onReconnectTimer);
}

bool Yeelight::isConnected()
{
    return m_socket->isOpen();
}

void Yeelight::connectDevice()
{
    m_socket->connectToHost(m_address, m_port);
}


int Yeelight::getParam(QList<Yeelight::YeelightProperty> properties)
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "get_prop";
    QJsonArray params;
    m_propertyRequests.insert(requestId, properties);

    foreach (YeelightProperty property, properties) {
        switch (property) {
        case YeelightProperty::Ct:
            params.append("ct");
            break;
        case YeelightProperty::Power:
            params.append("power");
            break;
        case YeelightProperty::Bright:
            params.append("bright");
            break;
        case YeelightProperty::Hue:
            params.append("hue");
            break;
        case YeelightProperty::Rgb:
            params.append("rgb");
            break;
        case YeelightProperty::Sat:
            params.append("sat");
            break;
        case YeelightProperty::Name:
            params.append("name");
            break;
        case YeelightProperty::BgCt:
            params.append("bg_ct");
            break;
        case YeelightProperty::NlBr:
            params.append("nl_br");
            break;
        case YeelightProperty::BgHue:
            params.append("bg_hue");
            break;
        case YeelightProperty::BgRgb:
            params.append("bg_rgb");
            break;
        case YeelightProperty::BgSat:
            params.append("bg_sat");
            break;
        case YeelightProperty::BgLmode:
            params.append("bg_lmode");
            break;
        case YeelightProperty::BgPower:
            params.append("bg_power");
            break;
        case YeelightProperty::Flowing:
            params.append("flowing");
            break;
        case YeelightProperty::MusicOn:
            params.append("music_on");
            break;
        case YeelightProperty::BgBright:
            params.append("bg_bright");
            break;
        case YeelightProperty::DelayOff:
            params.append("delay_off");
            break;
        case YeelightProperty::BgFlowing:
            params.append("bg_flowing");
            break;
        case YeelightProperty::ColorMode:
            params.append("color_mode");
            break;
        case YeelightProperty::ActiveMode:
            params.append("active_mode");
            break;
        case YeelightProperty::FlowParams:
            params.append("flow_params");
            break;
        case YeelightProperty::BgFlowParams:
            params.append("bg_flow_params");
            break;
        }
    }

    QTimer::singleShot(10000, this, [requestId, this]{m_propertyRequests.remove(requestId);});
    obj["params"] = params;
    doc.setObject(obj);
    //qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

int Yeelight::setName(const QString &name)
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "set_name";
    QJsonArray params;
    params.append(name);
    obj["params"] = params;
    doc.setObject(obj);
    //qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

int Yeelight::setColorTemperature(int mirad, int msFadeTime)
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "set_ct_abx";
    QJsonArray params;
    params.append(mirad);
    params.append("smooth");
    params.append(msFadeTime);
    obj["params"] = params;
    doc.setObject(obj);
    //qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

int Yeelight::setRgb(QRgb color, int msFadeTime)
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "set_rgb";
    QJsonArray params;
    params.append(static_cast<int>(color & 0x00ffffff));
    params.append("smooth");
    params.append(msFadeTime);
    obj["params"] = params;
    doc.setObject(obj);
    //qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

int Yeelight::setBrightness(int percentage, int msFadeTime)
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "set_bright";
    QJsonArray params;
    params.append(percentage);
    params.append("smooth");
    params.append(msFadeTime);
    obj["params"] = params;
    doc.setObject(obj);
    //qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

int Yeelight::setPower(bool power, int msFadeTime)
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "set_power";
    QJsonArray params;
    if(power) {
        params.append("on");
    } else {
        params.append("off");
    }
    params.append("smooth");
    params.append(msFadeTime);
    obj["params"] = params;
    doc.setObject(obj);
    //qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

int Yeelight::setDefault()
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "set_default";
    QJsonArray params;
    obj["params"] = params;
    doc.setObject(obj);
    //qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

int Yeelight::startColorFlow()
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "start_cf";
    QJsonArray params;
    params.append(0); //0 means infinite loop on the state changing
    params.append(0); //LED recover to the state before the color flow started
    params.append("2000, 1, 255, 50, 2000, 1, 5000, 50, 2000, 1, 6000, 50"); //Colors
    obj["params"] = params;
    doc.setObject(obj);
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

int Yeelight::stopColorFlow()
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "stop_cf";
    QJsonArray params;
    obj["params"] = params;
    doc.setObject(obj);
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

int Yeelight::flash()
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "start_cf";
    QJsonArray params;
    params.append(4 * 3);
    params.append(0); //LED recover to the state before the color flow started
    params.append("50, 2, 6500, 100, 500, 7, 6500, 1, 50, 2, 6500, 1, 500, 7, 6500, 1");
    obj["params"] = params;
    doc.setObject(obj);
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

int Yeelight::flash15s()
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "start_cf";
    QJsonArray params;
    params.append(4 * 15);
    params.append(0); //LED recover to the state before the color flow started
    params.append("50, 2, 6500, 100, 500, 7, 6500, 1, 50, 2, 6500, 1, 500, 7, 6500, 1");
    obj["params"] = params;
    doc.setObject(obj);
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
    m_socket->write(doc.toJson() + "\r\n");
    return requestId;
}

void Yeelight::onStateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::SocketState::ConnectedState:
        emit connectionChanged(true);
        break;
    case QAbstractSocket::SocketState::UnconnectedState:
        m_reconnectTimer->start(10 * 1000);
        emit connectionChanged(false);
        break;
    default:
        emit connectionChanged(false);
        break;
    }
}

void Yeelight::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    //qCDebug(dcYeelight()) << "Message received" << data;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug(dcYeelight()) << "Recieved invalide JSON object";
        return;
    }
    QVariantMap map = doc.toVariant().toMap();
    if (map.contains("error")) {
        if(map.contains("id")) {
            emit requestExecuted(map["id"].toInt(), false);
        }
        QVariantMap error = map["error"].toMap();
        int code = error["code"].toInt();
        QString message = error["message"].toString();
        qCWarning(dcYeelight()) << "Error received: Code" << code << message;
        emit errorReceived(code, message);

    }else if (map.contains("method")) {
        if (map["method"] == "props") {
            QVariantMap params = map["params"].toMap();
            if (params.contains("power")) {
                emit powerNotificationReceived((params["power"].toString() == "on"));
            }
            if (params.contains("bright")) {
                emit brightnessNotificationReceived(params["bright"].toInt());
            }
            if (params.contains("ct")) {
                emit colorTemperatureNotificationReceived(params["ct"].toInt());
            }
            if (params.contains("rgb")) {
                emit rgbNotificationReceived(QRgb(params["rgb"].toInt()));
            }
            if (params.contains("hue")) {
                emit hueNotificationReceived(params["hue"].toInt());
            }
            if (params.contains("name")) {
                emit nameNotificationReceived(params["name"].toString());
            }
            if (params.contains("color_mode")) {
                emit colorModeNotificationReceived(YeelightColorMode(params["color_mode"].toInt()));
            }
            if (params.contains("sat")) {
                emit saturationNotificationReceived(params["sat"].toInt());
            }
        }
    } else {
        int id = map["id"].toInt();
        QVariantList result = map["result"].toList();
        if ((result.length() == 1)) {
            if (result.first().toString() == "ok") {
                emit requestExecuted(id, true);
            }
        } else {
            if (m_propertyRequests.contains(id)) {
                QList<YeelightProperty> properties = m_propertyRequests.take(id);
                foreach (YeelightProperty property, properties) {
                    if (result.isEmpty()){
                        qCWarning(dcYeelight()) << "Value count does not match properties" << properties.count();
                        break;
                    }
                    QVariant value = result.takeFirst();
                    switch (property) {
                    case YeelightProperty::Name:
                         emit nameNotificationReceived(value.toString());
                    break;
                    case YeelightProperty::Ct:
                         emit colorTemperatureNotificationReceived(value.toInt());
                    break;
                    case YeelightProperty::Rgb:
                         emit rgbNotificationReceived(QRgb(value.toInt()));
                    break;
                    case YeelightProperty::Hue:
                         emit hueNotificationReceived(value.toInt());
                    break;
                    case YeelightProperty::Bright:
                         emit brightnessNotificationReceived(value.toInt());
                    break;
                    case YeelightProperty::Power:
                         emit powerNotificationReceived((value.toString() == "on"));
                    break;
                    case YeelightProperty::ColorMode:
                         emit colorModeNotificationReceived(YeelightColorMode(value.toInt()));
                    break;
                    case YeelightProperty::Sat:
                         emit saturationNotificationReceived(value.toInt());
                    break;
                    default:
                        qCWarning(dcYeelight()) << "Unhandled Yeelight property";
                    }
                }
            }
        }
    }
}

void Yeelight::onReconnectTimer()
{
    if(!m_socket->isOpen()) {
        m_socket->connectToHost(m_address, m_port);
        m_reconnectTimer->start(10 * 1000);
    }
}


