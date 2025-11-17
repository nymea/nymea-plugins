// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINSERIALPORTCOMMANDER_H
#define INTEGRATIONPLUGINSERIALPORTCOMMANDER_H

#include <integrations/integrationplugin.h>

#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>

class IntegrationPluginSerialPortCommander : public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationpluginserialportcommander.json")
    Q_INTERFACES(IntegrationPlugin)

public:
    explicit IntegrationPluginSerialPortCommander();

    void setupThing(ThingSetupInfo *info) override;
    void thingRemoved(Thing *thing) override;
    void discoverThings(ThingDiscoveryInfo *info) override;
    void executeAction(ThingActionInfo *info) override;

private:
    QTimer *m_reconnectTimer = nullptr;
    QHash<Thing *, QSerialPort *> m_serialPorts;

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);
    void onBaudRateChanged(qint32 baudRate, QSerialPort::Directions direction);
    void onParityChanged(QSerialPort::Parity parity);
    void onDataBitsChanged(QSerialPort::DataBits dataBits);
    void onStopBitsChanged(QSerialPort::StopBits stopBits);
    void onFlowControlChanged(QSerialPort::FlowControl flowControl);
    void onReconnectTimer();
};

#endif // INTEGRATIONPLUGINSERIALPORTCOMMANDER_H
