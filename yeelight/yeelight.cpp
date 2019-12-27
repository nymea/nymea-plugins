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
}

bool Yeelight::isConnected()
{
    return m_socket->isOpen();
}

void Yeelight::connectDevice()
{
    m_socket->connectToHost(m_address, m_port);
}


int Yeelight::getParam(QList<Yeelight::Property> properties)
{
    int requestId = static_cast<int>(QRandomGenerator::global()->generate());
    QJsonDocument doc;
    QJsonObject obj;
    obj["id"] = requestId;
    obj["method"] = "get_prop";
    QJsonArray params;

    foreach (Property property, properties) {
        switch (property) {
        case Property::Ct:
            params.append("ct");
            break;
        case Property::Power:
            params.append("power");
            break;
        case Property::Bright:
            params.append("bright");
            break;
        case Property::Hue:
            params.append("hue");
            break;
        case Property::Rgb:
            params.append("rgb");
            break;
        case Property::Sat:
            params.append("sat");
            break;
        case Property::Name:
            params.append("name");
            break;
        case Property::BgCt:
            params.append("bg_ct");
            break;
        case Property::NlBr:
            params.append("nl_br");
            break;
        case Property::BgHue:
            params.append("bg_hue");
            break;
        case Property::BgRgb:
            params.append("bg_rgb");
            break;
        case Property::BgSat:
            params.append("bg_sat");
            break;
        case Property::BgLmode:
            params.append("bg_lmode");
            break;
        case Property::BgPower:
            params.append("bg_power");
            break;
        case Property::Flowing:
            params.append("flowing");
            break;
        case Property::MusicOn:
            params.append("music_on");
            break;
        case Property::BgBright:
            params.append("bg_bright");
            break;
        case Property::DelayOff:
            params.append("delay_off");
            break;
        case Property::BgFlowing:
            params.append("bg_flowing");
            break;
        case Property::ColorMode:
            params.append("color_mode");
            break;
        case Property::ActiveMode:
            params.append("active_mode");
            break;
        case Property::FlowParams:
            params.append("flow_params");
            break;
        case Property::BgFlowParams:
            params.append("bg_flow_params");
            break;
        }
    }

    obj["params"] = params;
    doc.setObject(obj);
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
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
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
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
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
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
    params.append(QVariant(color).toInt());
    params.append("smooth");
    params.append(msFadeTime);
    obj["params"] = params;
    doc.setObject(obj);
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
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
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
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
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
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
    qCDebug(dcYeelight()) << "Sending request" << doc.toJson();
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
    params.append("2000, 1, 255, -1, 2000, 1, 16711680, -1, 2000, 1, 65280, -1,"); //Colors
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
    params.append(6);
    params.append(0); //LED recover to the state before the color flow started
    params.append("500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0,"); //Colors
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
    params.append(30);
    params.append(0); //LED recover to the state before the color flow started
    params.append("500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0, 500, 2, 4000, 100, 500, 7, 0, 0"); //Colors
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
    default:
        emit connectionChanged(false);
        break;
    }
}

void Yeelight::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    qCDebug(dcYeelight()) << "Message received" << data;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug(dcYeelight()) << "Recieved invalide JSON object";
        return;
    }
    QVariantMap map = doc.toVariant().toMap();
    if (map.contains("method")) {
        if (map["method"] == "props") {
            QVariantMap params = map["params"].toMap();
            if (params.contains("power")) {
                emit notificationReveiced(Property::Power, params["power"]);
            }
            if (params.contains("bright")) {
                emit notificationReveiced(Property::Bright, params["bright"]);
            }
            if (params.contains("ct")) {
                emit notificationReveiced(Property::Ct, params["ct"]);
            }
            if (params.contains("rgb")) {
                emit notificationReveiced(Property::Rgb, params["rgb"]);
            }
            if (params.contains("hue")) {
                emit notificationReveiced(Property::Hue, params["hue"]);
            }
            if (params.contains("name")) {
                emit notificationReveiced(Property::Name, params["name"]);
            }
            if (params.contains("color_mode")) {
                emit notificationReveiced(Property::ColorMode, params["color_mode"]);
            }
            if (params.contains("sat")) {
                emit notificationReveiced(Property::Sat, params["sat"]);
            }
        }
    } else {
        int id = map["id"].toInt();
        QVariantList result = map["result"].toList();
        if ((result.length() == 1)) {
            if (result.first().toString() == "ok") {
                emit requestExecuted(id, true);
            } else {
               //TODO parse error
              //emit errorReceived()
            }
        } else {
            emit propertyListReceived(result);
        }
    }
}


