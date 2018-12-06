/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Bernhard Trinnes <bernhard.trinnes@guh.io>          *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@guh.io>                   *
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

#ifndef DEVICEPLUGINUNIPI_H
#define DEVICEPLUGINUNIPI_H

#include "plugin/deviceplugin.h"
#include "devicemanager.h"
#include <QtWebSockets/QtWebSockets>
#include "plugintimer.h"
#include "dimmerswitch.h"

class DevicePluginUniPi : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginunipi.json")
    Q_INTERFACES(DevicePlugin)

public:

    explicit DevicePluginUniPi();
    ~DevicePluginUniPi();

    void init() override;
    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void deviceRemoved(Device *device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;
    DeviceManager::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;


private:

    enum GpioType {
        Relay,
        DigitalInput,
        DigitalOutput,
        AnalogInput,
        AnalogOutput
    };

    QHash<QString, Device*> m_usedRelais;
    QHash<QString, Device*> m_usedDigitalOutputs;
    QHash<QString, Device*> m_usedDigitalInputs;
    QHash<QString, Device*> m_usedAnalogOutputs;
    QHash<QString, Device*> m_usedAnalogInputs;
    QHash<QString, Device*> m_usedTemperatureSensors;
    QHash<QString, Device*> m_usedLeds;

    QHash<DimmerSwitch *, Device*> m_dimmerSwitches;

    QList<QString> m_relais;
    QList<QString> m_digitalOutputs;
    QList<QString> m_digitalInputs;
    QList<QString> m_analogOutputs;
    QList<QString> m_analogInputs;
    QList<QString> m_temperatureSensors;
    QList<QString> m_leds;

    QWebSocket *m_webSocket = nullptr;

    PluginTimer *m_refreshTimer = nullptr;

    void requestAllData();
    void setOutput(const QString &circuit, bool value);
    void connectToEvok();

private slots:
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketTextMessageReceived(QString message);
    void onRefreshTimer();

    void onDimmerSwitchPressed();
    void onDimmerSwitchLongPressed();
    void onDimmerSwitchDoublePressed();
    void onDimmerSwitchDimValueChanged(int dimValue);
};

#endif // DEVICEPLUGINUNIPI_H
