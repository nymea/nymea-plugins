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

#include "tvdevice.h"
#include "extern-plugininfo.h"

TvDevice::TvDevice(const QHostAddress &hostAddress, quint16 port, quint16 eventHandlerPort, QObject *parent) :
    QObject(parent),
    m_hostAddress(hostAddress),
    m_port(port),
    m_eventHandlerPort(eventHandlerPort)
{
    m_eventHandler = new TvEventHandler(hostAddress, eventHandlerPort, this);
    connect(m_eventHandler, &TvEventHandler::eventOccured, this, &TvDevice::eventOccured);

    // Init FS
    m_virtualFs = new VirtualFsNode(BrowserItem());

    // Apps
    BrowserItem item = BrowserItem("apps", tr("Apps"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconApplication);
    VirtualFsNode *apps = new VirtualFsNode(item);
    m_virtualFs->addChild(apps);

    // Channels
    item = BrowserItem("channels", tr("Channels"), true);
    item.setDescription(tr(""));
    item.setIcon(BrowserItem::BrowserIconVideo);
    VirtualFsNode *channels = new VirtualFsNode(item);
    m_virtualFs->addChild(channels);

}

void TvDevice::setKey(const QString &key)
{
    m_key = key;
}

QString TvDevice::key() const
{
    return m_key;
}

QHostAddress TvDevice::hostAddress() const
{
    return m_hostAddress;
}

quint16 TvDevice::port() const
{
    return m_port;
}

quint16 TvDevice::eventHandlerPort() const
{
    return m_eventHandlerPort;
}

void TvDevice::setUuid(const QString &uuid)
{
    m_uuid = uuid;
}

QString TvDevice::uuid() const
{
    return m_uuid;
}

void TvDevice::setPaired(const bool &paired)
{
    if (m_paired != paired) {
        m_paired = paired;
        emit stateChanged();
    }
}

bool TvDevice::paired() const
{
    return m_paired;
}

void TvDevice::setReachable(const bool &reachable)
{
    if (m_reachable != reachable) {
        qCDebug(dcLgSmartTv()) << "TV Event handler" << (reachable ? "reachable" : "not reachable any more");
        m_reachable = reachable;
        emit stateChanged();
    }
}

bool TvDevice::reachable() const
{
    return m_reachable;
}

bool TvDevice::is3DMode() const
{
    return m_is3DMode;
}

int TvDevice::volumeLevel() const
{
    return m_volumeLevel;
}

bool TvDevice::mute() const
{
    return m_mute;
}

QString TvDevice::channelType() const
{
    return m_channelType;
}

QString TvDevice::channelName() const
{
    return m_channelName;
}

int TvDevice::channelNumber() const
{
    return m_channelNumber;
}

QString TvDevice::programName() const
{
    return m_programName;
}

int TvDevice::inputSourceIndex() const
{
    return m_inputSourceIndex;
}

QString TvDevice::inputSourceLabelName() const
{
    return m_inputSourceLabel;
}

QPair<QNetworkRequest, QByteArray> TvDevice::createDisplayKeyRequest(const QHostAddress &host, quint16 port)
{
    QString urlString = "http://" + host.toString() + ":" + QString::number(port) + "/udap/api/pairing";
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=utf-8"));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("UDAP/2.0"));

    QByteArray data = "<?xml version=\"1.0\" encoding=\"utf-8\"?><envelope><api type=\"pairing\"> <name>showKey</name></api></envelope>";
    return QPair<QNetworkRequest, QByteArray>(request, data);
}

QPair<QNetworkRequest, QByteArray> TvDevice::createPairingRequest(const QHostAddress &host, quint16 port, quint16 eventHandlerPort, const QString &key)
{
    QString urlString = "http://" + host.toString() + ":" + QString::number(port) + "/udap/api/pairing";
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=utf-8"));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("UDAP/2.0 nymea"));

    QByteArray data = "<?xml version=\"1.0\" encoding=\"utf-8\"?><envelope><api type=\"pairing\"><name>hello</name><value>" +
            key.toUtf8() + "</value><port>" + QByteArray::number(eventHandlerPort) + "</port></api></envelope>";
    return QPair<QNetworkRequest, QByteArray>(request, data);
}

QPair<QNetworkRequest, QByteArray> TvDevice::createEndPairingRequest(const QHostAddress &host, quint16 port, quint16 eventHandlerPort)
{
    QString urlString = "http://" + host.toString() + ":" + QString::number(port) + "/udap/api/pairing";
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=utf-8"));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("UDAP/2.0 nymea"));
    request.setRawHeader("Connection", "Close");

    QByteArray data = "<?xml version=\"1.0\" encoding=\"utf-8\"?><envelope><api type=\"pairing\"><name>byebye</name><port>" + QByteArray::number(eventHandlerPort) + "</port></api></envelope>";
    return QPair<QNetworkRequest, QByteArray>(request, data);
}

QPair<QNetworkRequest, QByteArray> TvDevice::createEventRequest(const QHostAddress &host, quint16 port, quint16 eventHandlerPort)
{
    QString urlString = "http://" + host.toString() + ":" + QString::number(port) + "/udap/api/event";
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=utf-8"));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("UDAP/2.0 nymea"));
    request.setRawHeader("Connection", "Close");

    QByteArray data = "<?xml version=\"1.0\" encoding=\"utf-8\"?><envelope><api type=\"event\"><name>byebye</name><port>" + QByteArray::number(eventHandlerPort) + "</port></api></envelope>";
    return QPair<QNetworkRequest, QByteArray>(request, data);
}

QUrl TvDevice::buildDisplayImageUrl() const
{
    return QUrl("http://" + m_hostAddress.toString()  + ":" + QString::number(m_port) + "/udap/api/data?target=screen_image");
}

QNetworkRequest TvDevice::createVolumeInformationRequest()
{
    QString urlString = "http://" + hostAddress().toString()  + ":" + QString::number(port()) + "/udap/api/data?target=volume_info";
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=utf-8"));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("UDAP/2.0"));
    request.setRawHeader("Connection", "Close");
    return request;
}

QNetworkRequest TvDevice::createChannelListRequest()
{
    QString urlString = "http://" + hostAddress().toString()  + ":" + QString::number(port()) + "/udap/api/data?target=channel_list";
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=utf-8"));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("UDAP/2.0"));
    request.setRawHeader("Connection", "Close");
    return request;
}

QNetworkRequest TvDevice::createAppListCountRequest(int type)
{
    QString urlString = "http://" + hostAddress().toString()  + ":" + QString::number(port()) + "/udap/api/data?target=appnum_get&type=" + QString::number(type);
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=utf-8"));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("UDAP/2.0"));
    request.setRawHeader("Connection", "Close");
    return request;
}

QNetworkRequest TvDevice::createAppListRequest(int type, int appCount)
{
    QString urlString = "http://" + hostAddress().toString()  + ":" + QString::number(port()) + "/udap/api/data?target=applist_get&type=" + QString::number(type) + "&index=1&number=" + QString::number(appCount);
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=utf-8"));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("UDAP/2.0"));
    request.setRawHeader("Connection", "Close");
    return request;
}

QNetworkRequest TvDevice::createChannelInformationRequest()
{
    QString urlString = "http://" + hostAddress().toString()  + ":" + QString::number(port()) + "/udap/api/data?target=cur_channel";
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=utf-8"));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("UDAP/2.0"));
    request.setRawHeader("Connection", "Close");
    return request;
}

QPair<QNetworkRequest, QByteArray> TvDevice::createPressButtonRequest(const TvDevice::RemoteKey &key)
{
    QString urlString = "http://" + hostAddress().toString()  + ":" + QString::number(port()) + "/udap/api/command";
    QNetworkRequest request;
    request.setUrl(QUrl(urlString));
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("text/xml; charset=utf-8"));
    request.setHeader(QNetworkRequest::UserAgentHeader,QVariant("UDAP/2.0 nymea"));

    QByteArray data;
    data.append("<?xml version=\"1.0\" encoding=\"utf-8\"?><envelope><api type=\"command\"><name>HandleKeyInput</name><value>");
    data.append(QString::number(key).toUtf8());
    data.append("</value></api></envelope>");
    return QPair<QNetworkRequest, QByteArray>(request, data);
}

void TvDevice::onVolumeInformationUpdate(const QByteArray &data)
{
    //qCDebug(dcLgSmartTv()) << printXmlData(data);
    QXmlStreamReader xml(data);
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.name() == "mute") {
            m_mute = QVariant(xml.readElementText()).toBool();
        }
        if (xml.name() == "level") {
            m_volumeLevel = QVariant(xml.readElementText()).toInt();
        }
    }
    emit stateChanged();
}

void TvDevice::onChannelInformationUpdate(const QByteArray &data)
{
    //qCDebug(dcLgSmartTv()) << printXmlData(data);
    QXmlStreamReader xml(data);

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.name() == "chtype") {
            m_channelType = xml.readElementText();
        }
        if (xml.name() == "major") {
            m_channelNumber = QVariant(xml.readElementText()).toInt();
        }
        if (xml.name() == "chname") {
            m_channelName = xml.readElementText();
        }
        if (xml.name() == "progName") {
            m_programName = xml.readElementText();
        }
        if (xml.name() == "inputSourceIdx") {
            m_inputSourceIndex = QVariant(xml.readElementText()).toInt();
        }
        if (xml.name() == "labelName") {
            m_inputSourceLabel = xml.readElementText();
        }
    }
    emit stateChanged();
}

void TvDevice::onAppListUpdated(const QByteArray &appList)
{
    Q_UNUSED(appList)
}

QString TvDevice::printXmlData(const QByteArray &data)
{
    QString xmlOut;
    QXmlStreamReader reader(data);
    QXmlStreamWriter writer(&xmlOut);
    writer.setAutoFormatting(true);
    while (!reader.atEnd()) {
        reader.readNext();
        if(!reader.isWhitespace()) {
            writer.writeCurrentToken(reader);
        }
    }
    if (reader.hasError()) {
        qCWarning(dcLgSmartTv()) << "Error reading XML thing information:" << reader.errorString();
    }
    return xmlOut;
}

void TvDevice::browse(BrowseResult *result)
{
    qCDebug(dcLgSmartTv()) << "Browse" << result->itemId();
    VirtualFsNode *node = m_virtualFs->findNode(result->itemId());
    if (node) {
        if (result->itemId().isEmpty()) {
            foreach (VirtualFsNode *child, node->childs) {
                result->addItem(child->item);
            }
            result->finish(Thing::ThingErrorNoError);
            return;
        }

        if (result->itemId().startsWith("apps")) {
            qCDebug(dcLgSmartTv()) << m_appList;
            // Build the app list
            foreach (const QVariant &appVariant, m_appList) {
                QVariantMap appMap = appVariant.toMap();
                qCDebug(dcLgSmartTv()) << "Entry:" << appMap;
                BrowserItem item("apps:" + appMap.value("name").toString(), appMap.value("name").toString());
                item.setExecutable(true);
                item.setIcon(BrowserItem::BrowserIconApplication);
                result->addItem(item);
            }
            result->finish(Thing::ThingErrorNoError);

        } else if (result->itemId().startsWith("channels")) {
            // Build the channel list

        }
    }


}

void TvDevice::browserItem(BrowserItemResult *result)
{
    qCDebug(dcLgSmartTv()) << "Browse item" << result->itemId();

}

int TvDevice::launchBrowserItem(const QString &itemId)
{
    qCDebug(dcLgSmartTv()) << "Launch browse item" << itemId;

    // TODO: switch channel or launch app
    return 0;
}

void TvDevice::clearAppList()
{
    m_appList.clear();
}

void TvDevice::addAppList(const QVariantList &appList)
{
    foreach (const QVariant &appVariant, appList) {
        m_appList.append(appVariant);
    }
}

void TvDevice::eventOccured(const QByteArray &data)
{
    qCDebug(dcLgSmartTv()) << "Event handler data received" << qUtf8Printable(printXmlData(data));

    // if we got a channel changed event...
    if (data.contains("ChannelChanged")) {
        onChannelInformationUpdate(data);
        return;
    }

    //TODO: handle ip address change (dhcp) notification from the tv!

    // if the tv suspends, it will send a byebye message, which means
    // the pairing will be closed.
    if (data.contains("api type=\"pairing\"") && data.contains("byebye")) {
        qCDebug(dcLgSmartTv()) << "Ended pairing (host)";
        setPaired(false);
        setReachable(false);
        return;
    }

    // check if this is a 3DMode changed event
    QXmlStreamReader xml(data);
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.name() == "name") {
            if (xml.readElementText() == "3DMode") {
                xml.readNext();
                if (xml.name() == "value") {
                    m_is3DMode = QVariant(xml.readElementText()).toBool();
                }
            }
        }
    }
    emit stateChanged();
}
