/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@nymea.io>                 *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#ifndef DISCOVER_H
#define DISCOVER_H

#include <QObject>
#include <QProcess>
#include <QXmlStreamReader>
#include <QTimer>
#include <QHostAddress>

#include "host.h"

class Discover : public QObject
{
    Q_OBJECT
public:

    explicit Discover(QStringList arguments, QObject *parent = nullptr);
    ~Discover();
    bool getProcessStatus();

private:
    QTimer *m_timer;
    QStringList m_arguments;

    QProcess * m_discoveryProcess;
    QProcess * m_scanProcess;

    QXmlStreamReader m_reader;

    bool m_aboutToQuit;

    QStringList getDefaultTargets();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // Process parsing
    QList<Host> parseProcessOutput(const QByteArray &processData);
    Host parseHost();

signals:
    void devicesDiscovered(QList<Host>);

private slots:
    void onTimeout();
};

#endif // DISCOVER_H
