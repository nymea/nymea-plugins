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

#include "heatpump.h"
#include "coap/corelinkparser.h"

#include "extern-plugininfo.h"

HeatPump::HeatPump(QHostAddress address, QObject *parent) :
    QObject(parent),
    m_address(address),
    m_reachable(false),
    m_sgMode(0)
{
    m_coap = new Coap(this);
    connect(m_coap, SIGNAL(replyFinished(CoapReply*)), this, SLOT(onReplyFinished(CoapReply*)));

    QUrl url;
    url.setScheme("coap");
    url.setHost(m_address.toString());
    url.setPath("/.well-known/core");

    qCDebug(dcAwattar) << "Discover pump resources on" << url.toString();
    CoapReply *reply = m_coap->get(CoapRequest(url));
    if (reply->error() != CoapReply::NoError) {
        qCWarning(dcAwattar()) << "Could not discover pump resources" << reply->errorString();
        reply->deleteLater();
        return;
    }
    m_discoverReplies.append(reply);
}

QHostAddress HeatPump::address() const
{
    return m_address;
}

bool HeatPump::reachable() const
{
    return m_reachable;
}

void HeatPump::setSgMode(const int &sgMode)
{    
    // Note: always try to set sg-mode, to make sure the pump is still reachable (like a ping)
    if (m_sgMode != sgMode) {
        m_sgMode = sgMode;
        qCDebug(dcAwattar) << "Setting sg-mode to" << sgMode;
    }

    QUrl url;
    url.setScheme("coap");
    url.setHost(m_address.toString());
    url.setPath("/a/sg_mode");

    QByteArray payload = QString("mode=%1").arg(QString::number(m_sgMode)).toUtf8();

    CoapReply *reply = m_coap->post(CoapRequest(url), payload);
    if (reply->error() != CoapReply::NoError) {
        qCWarning(dcAwattar()) << "Could not set sg mode" << reply->errorString();
        setReachable(false);
        reply->deleteLater();
        return;
    }

    m_sgModeReplies.append(reply);
}

void HeatPump::setReachable(const bool &reachable)
{
    if (m_reachable != reachable) {
        m_reachable = reachable;
        emit reachableChanged();
    }
}

void HeatPump::onReplyFinished(CoapReply *reply)
{
    if (m_discoverReplies.contains(reply)) {
        m_discoverReplies.removeAll(reply);

        if (reply->error() != CoapReply::NoError) {
            qCWarning(dcAwattar()) << "CoAP resource discovery reply error" << reply->errorString();
            setReachable(false);
            reply->deleteLater();
            return;
        }

        if (reply->statusCode() != CoapPdu::Content) {
            qCWarning(dcAwattar()) << "Resource discovery status code:" << reply;
            setReachable(false);
            reply->deleteLater();
            return;
        }

        qCDebug(dcAwattar) << "Discovered successfully the resources";
        CoreLinkParser parser(reply->payload());
        foreach (const CoreLink &link, parser.links()) {
            qCDebug(dcAwattar) << link << endl;
        }

    } else if (m_sgModeReplies.contains(reply)) {
        m_sgModeReplies.removeAll(reply);

        if (reply->error() != CoapReply::NoError) {
            if (reachable())
                qCWarning(dcAwattar()) << "CoAP sg-mode reply error" << reply->errorString();

            setReachable(false);
            reply->deleteLater();
            return;
        }

        if (reply->statusCode() != CoapPdu::Content) {
            qCWarning(dcAwattar()) << "Set sg-mode status code error:" << reply;
            setReachable(false);
            reply->deleteLater();
            return;
        }

        if (!reachable())
            qCDebug(dcAwattar) << "Set sg-mode successfully.";

    }

    // the reply had no error until now, so make sure the resource is reachable
    setReachable(true);
    reply->deleteLater();
}
