/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@guh.guru>                *
 *  Copyright (C) 2014 Michael Zanetti <michael_zanetti@gmx.net>           *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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
    explicit BobClient(const QString &host = "127.0.0.1", const int &port = 19333, QObject *parent = 0);
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
