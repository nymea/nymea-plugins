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

#include "webosconnection.h"
#include "extern-plugininfo.h"

#include <QJsonDocument>

WebosConnection::WebosConnection(QObject *parent) :
    QObject(parent)
{
    m_websocket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);

    connect(m_websocket, &QWebSocket::connected, this, &WebosConnection::onConnected);
    connect(m_websocket, &QWebSocket::disconnected, this, &WebosConnection::onDisconnected);
    connect(m_websocket, &QWebSocket::stateChanged, this, &WebosConnection::onStateChanged);
    connect(m_websocket, &QWebSocket::textMessageReceived, this, &WebosConnection::onTextMessageReceived);
}

QHostAddress WebosConnection::hostAddress() const
{
    return m_hostAddress;
}

void WebosConnection::setHostAddress(const QHostAddress &hostAddress)
{
    m_hostAddress = hostAddress;
}

bool WebosConnection::connected() const
{
    return m_websocket->state() == QAbstractSocket::ConnectedState;
}

void WebosConnection::sendRequest(const QVariantMap &request)
{
    m_websocket->sendTextMessage(QJsonDocument::fromVariant(request).toJson());
    m_id++;
}

void WebosConnection::getVolume()
{
    qCDebug(dcLgSmartTv()) << "WebOS get volume";
    // {"type":"request","id":"status_' + i +'","uri":"ssap://audio/getVolume"}
    QVariantMap request;
    request.insert("type", "request");
    request.insert("id", QString("status_%1").arg(m_id));
    request.insert("uri", "ssap://audio/getVolume");

    // TODO: make async and match request with respone id
    sendRequest(request);
}

void WebosConnection::onConnected()
{
    qCDebug(dcLgSmartTv()) << "WebOS connected to" << m_hostAddress.toString();
    emit connectedChanged(true);
    m_id = 0;

    // Register the client and init the current values
    registerClient();
    getVolume();
}

void WebosConnection::onDisconnected()
{
    qCDebug(dcLgSmartTv()) << "WebOS disconnected from" << m_hostAddress.toString() << m_websocket->closeReason();
    emit connectedChanged(false);
}

void WebosConnection::onStateChanged(const QAbstractSocket::SocketState &state)
{
    qCDebug(dcLgSmartTv()) << "WebOS connection state changed" << state;
}

void WebosConnection::onTextMessageReceived(const QString &message)
{
    qCDebug(dcLgSmartTv()) << "WebOS message received" << message;

    // TODO: parse data and update stuff
}

void WebosConnection::registerClient()
{
    //    QVariantMap request;
    //    request.insert("type", "request");
    //    request.insert("id", QString("register_%1").arg(m_id));

    //    QVariantMap payload;
    //    payload.insert("forcePairing", false);
    //    payload.insert("pairingType", "PROMPT");

    //    QVariantMap manifest;
    //    manifest.insert("manifestVersion", 1);
    //    manifest.insert("appVersion", "1.1");
    //    payload.insert("manifest", manifest);

    //    request.insert("payload", payload);

    //    request.insert("uri", "ssap://audio/getVolume");

    // Note: this is a static string copied from the internet
    QByteArray requestData("{\"type\":\"register\",\"id\":\"register_0\",\"payload\":{\"forcePairing\":false,\"pairingType\":\"PROMPT\",\"manifest\":{\"manifestVersion\":1,\"appVersion\":\"1.1\",\"signed\":{\"created\":\"20140509\",\"appId\":\"com.lge.test\",\"vendorId\":\"com.lge\",\"localizedAppNames\":{\"\":\"LG Remote App\",\"ko-KR\":\"리모컨 앱\",\"zxx-XX\":\"ЛГ Rэмotэ AПП\"},\"localizedVendorNames\":{\"\":\"LG Electronics\"},\"permissions\":[\"TEST_SECURE\",\"CONTROL_INPUT_TEXT\",\"CONTROL_MOUSE_AND_KEYBOARD\",\"READ_INSTALLED_APPS\",\"READ_LGE_SDX\",\"READ_NOTIFICATIONS\",\"SEARCH\",\"WRITE_SETTINGS\",\"WRITE_NOTIFICATION_ALERT\",\"CONTROL_POWER\",\"READ_CURRENT_CHANNEL\",\"READ_RUNNING_APPS\",\"READ_UPDATE_INFO\",\"UPDATE_FROM_REMOTE_APP\",\"READ_LGE_TV_INPUT_EVENTS\",\"READ_TV_CURRENT_TIME\"],\"serial\":\"2f930e2d2cfe083771f68e4fe7bb07\"},\"permissions\":[\"LAUNCH\",\"LAUNCH_WEBAPP\",\"APP_TO_APP\",\"CLOSE\",\"TEST_OPEN\",\"TEST_PROTECTED\",\"CONTROL_AUDIO\",\"CONTROL_DISPLAY\",\"CONTROL_INPUT_JOYSTICK\",\"CONTROL_INPUT_MEDIA_RECORDING\",\"CONTROL_INPUT_MEDIA_PLAYBACK\",\"CONTROL_INPUT_TV\",\"CONTROL_POWER\",\"READ_APP_STATUS\",\"READ_CURRENT_CHANNEL\",\"READ_INPUT_DEVICE_LIST\",\"READ_NETWORK_STATE\",\"READ_RUNNING_APPS\",\"READ_TV_CHANNEL_LIST\",\"WRITE_NOTIFICATION_TOAST\",\"READ_POWER_STATE\",\"READ_COUNTRY_INFO\"],\"signatures\":[{\"signatureVersion\":1,\"signature\":\"eyJhbGdvcml0aG0iOiJSU0EtU0hBMjU2Iiwia2V5SWQiOiJ0ZXN0LXNpZ25pbmctY2VydCIsInNpZ25hdHVyZVZlcnNpb24iOjF9.hrVRgjCwXVvE2OOSpDZ58hR+59aFNwYDyjQgKk3auukd7pcegmE2CzPCa0bJ0ZsRAcKkCTJrWo5iDzNhMBWRyaMOv5zWSrthlf7G128qvIlpMT0YNY+n/FaOHE73uLrS/g7swl3/qH/BGFG2Hu4RlL48eb3lLKqTt2xKHdCs6Cd4RMfJPYnzgvI4BNrFUKsjkcu+WD4OO2A27Pq1n50cMchmcaXadJhGrOqH5YmHdOCj5NSHzJYrsW0HPlpuAx/ECMeIZYDh6RMqaFM2DXzdKX9NmmyqzJ3o/0lkk/N97gfVRLW5hA29yeAwaCViZNCP8iC9aO0q9fQojoa7NQnAtw==\"}]}}}");
    sendRequest(QJsonDocument::fromJson(requestData).toVariant().toMap());
}

void WebosConnection::connectTv()
{
    QUrl url;
    url.setScheme("ws");
    url.setHost(m_hostAddress.toString());
    url.setPort(3000);

    qCDebug(dcLgSmartTv()) << "Connecting to WebOS" << url.toString();

    m_websocket->open(url);
}
