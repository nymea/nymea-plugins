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

#ifndef BOBCLIENT_H
#define BOBCLIENT_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QColor>
#include <QTime>

#include <bobchannel.h>

class BobClient : public QObject
{
    Q_OBJECT
public:
    explicit BobClient(const QString &host = "127.0.0.1", const int &port = 19333, QObject *parent = nullptr);
    ~BobClient();

    bool connectToBoblight();
    bool connected();

    int lightsCount();
    QColor currentColor(const int &channel);

    void setPriority(int priority);

    void setPower(int channel, bool power);
    void setColor(int channel, QColor color);
    void setBrightness(int channel, int brightness);

private:
    void *m_boblight = nullptr;

    QTimer *m_syncTimer;
    QString m_host;
    int m_port;
    bool m_connected;
    int m_priority = 128;

    QMap<int, QColor> m_colors;
    QMap<int, BobChannel *> m_channels;

    BobChannel *getChannel(const int &id);


private slots:
    void sync();
    void setConnected(bool connected);

signals:
    void connectionChanged();
    void powerChanged(int channel, bool power);
    void brightnessChanged(int channel, int brightness);
    void colorChanged(int channel, const QColor &color);
    void priorityChanged(int priority);
};

#endif // BOBCLIENT_H
