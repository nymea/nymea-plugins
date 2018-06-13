/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes  <bernhard.trinnes@guh.io>         *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef SERIALPORTCOMMANDER_H
#define SERIALPORTCOMMANDER_H

#include <QObject>
#include <QSerialPort>
#include "extern-plugininfo.h"
#include "devicemanager.h"

class SerialPortCommander : public QObject
{
    Q_OBJECT
public:
    explicit SerialPortCommander(QSerialPort *serialPort ,QObject *parent = 0);
    ~SerialPortCommander();

    enum ComparisonType {
        IsExactly,
        Contains,
        ContainsNot,
        StartsWith,
        EndsWith
    };

    void addOutputDevice(Device *device);
    void addInputDevice(Device *device);
    void removeInputDevice(Device *device);
    bool isEmpty();
    bool hasOutputDevice();
    void removeOutputDevice();
    void sendCommand(QByteArray data);
    QSerialPort *serialPort();
    Device *outputDevice();

private:
    QList<Device *> m_inputDevices;
    Device *m_outputDevice;
    QSerialPort *m_serialPort;

signals:
    void commandReceived(Device *device);

public slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);

};

#endif // SERIALPORTCOMMANDER_H
