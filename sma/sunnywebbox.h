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

#ifndef SUNNYWEBBOX_H
#define SUNNYWEBBOX_H

#include "integrations/thing.h"
#include "sunnywebboxcommunication.h"

#include <QObject>
#include <QHostAddress>
#include <QUdpSocket>

class SunnyWebBox : public QObject
{
    Q_OBJECT

public:
    struct Overview {
        int power;
        double dailyYield;
        int totalYield;
        QString status;
        QString error;
    };

    struct Device {
        QString key;
        QString name;
        QList<Device> childrens;
    };

    struct Channel {
        QString meta;
        QString name;
        QVariant value;
        QString unit;
    };

    struct Parameter {
        QString meta;
        QString name;
        QString unit;
        double min;
        double max;
        double value;
    };

    explicit SunnyWebBox(NetworkAccessManager *networkAccessManager, const QHostAddress &hostAddress, QObject *parrent = 0);
    ~SunnyWebBox();

    QString getPlantOverview(); // Returns an object with the following plant data: PAC, E-TODAY, E-TOTAL, MODE, ERROR
    QString getDevices(); //Returns a hierarchical list of all detected plant devices.
    QString getProcessDataChannels(const QString &deviceKey); //Returns a list with the meta names of the available process data channels for a particular device type.
    QString getProcessData(const QStringList &deviceKeys); //Returns process data for up to 5 devices per request.
    QString getParameterChannels(const QString &deviceKey); //Returns a list with the meta names of the available parameter channels for a particular device type
    QString getParameters(const QStringList &deviceKeys); //Returns the parameter values of up to 5 devices
    QString setParameters(const QString &deviceKeys, const QHash<QString, QVariant> &channels); //Sets parameter values

    void setHostAddress(const QHostAddress &address);
    QHostAddress hostAddress();

private:

    QHostAddress m_hostAddresss;
    NetworkAccessManager *m_networkManager = nullptr;

    QString sendMessage(const QHostAddress &address, const QString &procedure);
    QString sendMessage(const QHostAddress &address, const QString &procedure, const QJsonObject &params);
    void parseMessage(const QString &messageId, const QString &messageType, const QVariantMap &result);

signals:
    void connectedChanged(bool connected);

    void plantOverviewReceived(const QString &messageId, Overview overview);
    void devicesReceived(const QString &messageId, QList<Device> devices);
    void processDataChannelsReceived(const QString &messageId, const QString &deviceKey, QStringList processDataChanels);
    void processDataReceived(const QString &messageId, const QString &deviceKey, const QHash<QString, QVariant> &channels);
    void parameterChannelsReceived(const QString &messageId, const QString &deviceKey, QStringList parameterChannels);
    void parametersReceived(const QString &messageId, const QString &deviceKey, const QList<Parameter> &parameters);
};

#endif // SUNNYWEBBOX_H
