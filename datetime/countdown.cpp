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

#include "countdown.h"
#include "extern-plugininfo.h"

Countdown::Countdown(const QString &name, const QTime &time, const bool &repeating, QObject *parent) :
    QObject(parent),
    m_name(name),
    m_time(time),
    m_currentTime(time),
    m_repeating(repeating)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    m_timer->setSingleShot(false);

    connect(m_timer, &QTimer::timeout, this, &Countdown::onTimeout);
}

void Countdown::start()
{
    qCDebug(dcDateTime) << name() << "start" << m_currentTime.toString();
    m_currentTime = m_time;
    m_timer->start();
    m_running = true;
    emit runningStateChanged(true);
}

void Countdown::stop()
{
    m_timer->stop();
    qCDebug(dcDateTime) << name() << "stop" << m_currentTime.toString();
    m_running = false;
    emit runningStateChanged(false);
}

void Countdown::restart()
{
    m_currentTime = m_time;
    qCDebug(dcDateTime) << name() << "restart" << m_currentTime.toString();
    m_timer->start();
    m_running = true;
    emit runningStateChanged(true);
}

QString Countdown::name() const
{
    return m_name;
}

bool Countdown::running() const
{
    return m_running;
}

bool Countdown::repeating() const
{
    return m_repeating;
}

QTime Countdown::time() const
{
    return m_time;
}

QTime Countdown::currentTime() const
{
    return m_currentTime;
}

void Countdown::onTimeout()
{
    m_currentTime = m_currentTime.addSecs(-1);
    //qCDebug(dcDateTime) << name() << m_currentTime.toString();
    if (m_currentTime == QTime(0,0)) {
        qCDebug(dcDateTime) << name() << "countdown timeout.";
        if (m_repeating) {
            m_currentTime = m_time;
        } else {
            m_timer->stop();
            m_running = false;
            emit runningStateChanged(false);
        }
        emit countdownTimeout();
    }
}
