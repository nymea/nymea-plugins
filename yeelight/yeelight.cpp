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

#include <QRandomGenerator>

Yeelight::Yeelight(NetworkAccessManager *networkManager, const QHostAddress &address, quint16 port, QObject *parent) :
    QObject(parent),
    m_address(address),
    m_port(port),
    m_networkManager(networkManager)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::stateChanged, this, &Yeelight::onStateChanged);
    m_socket->connectToHost(address, port);
}

bool Yeelight::connected()
{
    return m_socket->isOpen();
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
            params.append("powr");
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
    m_socket->write(doc.toJson());
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
    return requestId;
}

void Yeelight::onStateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::SocketState::ConnectedState:

        break;
    default:
        break;
    }
}


