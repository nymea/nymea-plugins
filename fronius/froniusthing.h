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

#ifndef FRONIUSTHING_H
#define FRONIUSTHING_H

#include "extern-plugininfo.h"
#include "integrations/thing.h"

#include <QObject>
#include <QUrl>
#include <QUrlQuery>

class FroniusThing : public QObject
{
    Q_OBJECT

public:
    explicit FroniusThing(Thing *thing, QObject *parrent = 0);

    QString name() const;
    void setName(const QString &name);

    QString hostId() const;
    void setHostId(const QString &hostId);

    QString hostAddress() const;
    void setHostAddress(const QString &hostAddress);

    QString baseUrl() const;
    void setBaseUrl(const QString &baseUrl);

    QString uniqueId() const;
    void setUniqueId(const QString &uniqueId);

    QString deviceId() const;
    void setDeviceId(const QString &deviceId);

    Thing* pluginThing() const;

private:

    Thing* m_thing;

    QString m_name;
    QString m_hostId;
    QString m_hostAddress;
    QString m_apiVersion;
    QString m_baseUrl;
    QString m_uniqueId;
    QString m_thingId;

};

#endif // FRONIUSTHING_H
