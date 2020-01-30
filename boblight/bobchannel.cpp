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

#include "bobchannel.h"

BobChannel::BobChannel(const int &id, QObject *parent) :
    QObject(parent),
    m_id(id)
{
    m_animation = new QPropertyAnimation(this, "finalColor", this);
    m_animation->setDuration(500);
}

int BobChannel::id() const
{
    return m_id;
}

QColor BobChannel::color() const
{
    return m_color;
}

void BobChannel::setColor(const QColor &color)
{
    if (m_animation->state() == QPropertyAnimation::Running) {
        m_animation->stop();
        m_finalColor = m_color;
    }

    m_color = color;
    emit colorChanged();
    
    m_animation->setStartValue(m_finalColor);
    m_animation->setEndValue(color);
    m_animation->start();
}

bool BobChannel::power() const
{
    return m_power;
}

void BobChannel::setPower(bool power)
{
    if (power != m_power) {
        m_power = power;
        emit powerChanged();

        if (m_animation->state() == QPropertyAnimation::Running) {
            m_animation->stop();
        }

        QColor target = m_color;
        target.setAlpha(target.alpha() * (m_power ? 1 : 0));
        m_animation->setStartValue(m_finalColor);
        m_animation->setEndValue(target);
        m_animation->start();
    }
}

QColor BobChannel::finalColor() const
{
    return m_finalColor;
}

void BobChannel::setFinalColor(const QColor &color)
{
    m_finalColor = color;
    emit finalColorChanged();
}

