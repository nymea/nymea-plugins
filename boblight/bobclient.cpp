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

#include "bobclient.h"
#include "extern-plugininfo.h"

#include "libboblight/boblight.h"

#include <QDebug>
#include <QtConcurrent>

BobClient::BobClient(const QString &host, const int &port, QObject *parent) :
    QObject(parent),
    m_host(host),
    m_port(port),
    m_connected(false)
{
    m_syncTimer = new QTimer(this);
    m_syncTimer->setSingleShot(false);
    m_syncTimer->setInterval(25);

    connect(m_syncTimer, SIGNAL(timeout()), this, SLOT(sync()));
}

BobClient::~BobClient()
{
    if (m_boblight) {
        boblight_destroy(m_boblight);
    }
}

bool BobClient::connectToBoblight()
{
    if (connected()) {
        return true;
    }
    m_boblight = boblight_init();

    //try to connect, if we can't then bitch to stderr and destroy boblight
    if (!boblight_connect(m_boblight, m_host.toLatin1().data(), m_port, 1000000)) {
        qCWarning(dcBoblight) << "Failed to connect:" << boblight_geterror(m_boblight);
        boblight_destroy(m_boblight);
        m_boblight = nullptr;
        setConnected(false);
        return false;
    }

    qCDebug(dcBoblight) << "Connected to boblightd successfully.";
    boblight_setpriority(m_boblight, m_priority);
    for (int i = 0; i < lightsCount(); ++i) {
        BobChannel *channel = new BobChannel(i, this);
        channel->setColor(QColor(255,255,255,0));
        connect(channel, SIGNAL(colorChanged()), this, SLOT(sync()));
        m_channels.insert(i, channel);
    }
    setConnected(true);
    return true;
}

bool BobClient::connected()
{
    return m_connected;
}

void BobClient::setPriority(int priority)
{
    m_priority = priority;
    if (connected()) {
        qCDebug(dcBoblight) << "setting priority to" << priority;
        boblight_setpriority(m_boblight, priority);
    }
    emit priorityChanged(priority);
}

void BobClient::setPower(int channel, bool power)
{
    qCDebug(dcBoblight()) << "BobClient: setPower" << channel << power;
    m_channels.value(channel)->setPower(power);
    emit powerChanged(channel, power);
}

BobChannel *BobClient::getChannel(const int &id)
{
    foreach (BobChannel *channel, m_channels) {
        if (channel->id() == id)
            return channel;
    }
    return nullptr;
}

void BobClient::setColor(int channel, QColor color)
{    
    if (channel == -1) {
        for (int i = 0; i < lightsCount(); ++i) {
            setColor(i, color);
        }
    } else {
        BobChannel *c = getChannel(channel);
        if (c) {
            c->setColor(color);
            qCDebug(dcBoblight) << "set channel" << channel << "to color" << color;
            emit colorChanged(channel, color);
        }
    }
}

void BobClient::setBrightness(int channel, int brightness)
{
    QColor color = m_channels.value(channel)->color();
    color.setAlpha(qRound(brightness * 255.0 / 100));
    m_channels.value(channel)->setColor(color);
    emit brightnessChanged(channel, brightness);

    if (brightness > 0) {
        m_channels.value(channel)->setPower(true);
        emit powerChanged(channel, true);
    }
}

void BobClient::sync()
{
    if (!m_connected)
        return;

    foreach (BobChannel *channel, m_channels) {
        int rgb[3];
        rgb[0] = channel->finalColor().red() * channel->finalColor().alphaF();
        rgb[1] = channel->finalColor().green() * channel->finalColor().alphaF();
        rgb[2] = channel->finalColor().blue() * channel->finalColor().alphaF();
        boblight_addpixel(m_boblight, channel->id(), rgb);
    }

    if (!boblight_sendrgb(m_boblight, 1, nullptr)) {
        qCWarning(dcBoblight) << "Boblight connection error:" << boblight_geterror(m_boblight);
        boblight_destroy(m_boblight);
        qDeleteAll(m_channels);
        m_channels.clear();
        setConnected(false);
    }
}

void BobClient::setConnected(bool connected)
{
    m_connected = connected;
    emit connectionChanged();

    // if disconnected, delete all channels
    if (!connected) {
        m_syncTimer->stop();
        qDeleteAll(m_channels);
    } else {
        m_syncTimer->start();
    }
}

int BobClient::lightsCount()
{
    return boblight_getnrlights(m_boblight);
}

QColor BobClient::currentColor(const int &channel)
{
    return getChannel(channel)->color();
}
