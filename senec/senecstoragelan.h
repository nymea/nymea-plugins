/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#ifndef SENECSTORAGELAN_H
#define SENECSTORAGELAN_H

#include <QUrl>
#include <QObject>
#include <QHostAddress>

#include <network/networkaccessmanager.h>

class SenecStorageLan : public QObject
{
    Q_OBJECT
public:
    explicit SenecStorageLan(NetworkAccessManager *networkManager, QObject *parent = nullptr);
    explicit SenecStorageLan(NetworkAccessManager *networkManager, const QHostAddress &address, QObject *parent = nullptr);

    QHostAddress address() const;
    void setAddress(const QHostAddress &address);

    QUrl url() const;

    bool available() const;

    QString deviceId() const;
    float capacity() const;
    float maxChargePower() const;
    float maxDischargePower() const;


    static float parseFloat(const QString &value);
    static QString parseString(const QString &value);


public slots:
    void initialize();

signals:
    void initializeFinished(bool success);
    void availableChanged(bool available);

private:
    NetworkAccessManager *m_networkManager = nullptr;

    QUrl m_url;
    QHostAddress m_address;

    bool m_available = false;

    QString m_deviceId;
    float m_capacity = 0;
    float m_maxChargePower = 0;
    float m_maxDischargePower = 0;

    void updateUrl();
    void setAvailable(bool available);


private slots:
    void ignoreSslErrors(const QList<QSslError> &errors);

};

#endif // SENECSTORAGELAN_H
