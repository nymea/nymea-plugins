/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *  Copyright (C) 2018 Simon Stürz <simon.stuerz@nymea.io>                 *
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
#include "plugintimer.h"
#include "dimmerswitch.h"
#include "modbusrtumaster.h"
#include "modbustcpmaster.h"

#include <QtWebSockets/QtWebSockets>

class DevicePluginUniPi : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginunipi.json")
    Q_INTERFACES(DevicePlugin)

public:

    explicit DevicePluginUniPi();

    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;
    void deviceRemoved(Device *device) override;

private:

    enum GpioType {
        Relay,
        DigitalInput,
        DigitalOutput,
        AnalogInput,
        AnalogOutput
    };

    QHash<DimmerSwitch *, Device*> m_dimmerSwitches;
    PluginTimer *m_refreshTimer = nullptr;
    ModbusTCPMaster *m_modbusTCPMaster = nullptr;
    QString m_neuronModel;

    void setDigitalOutput(const QString &circuit, bool value);
    bool getDigitalOutput(const QString &circuit);

    void setExtensionDigitalOutput(const QString &circuit, bool value);
    bool getExtensionDigitalOutput(const QString &circuit);
    bool getExtensionDigitalInput(const QString &circuit);

private slots:
    void onRefreshTimer();

    void onDimmerSwitchPressed();
    void onDimmerSwitchLongPressed();
    void onDimmerSwitchDoublePressed();
    void onDimmerSwitchDimValueChanged(int dimValue);
};

#endif // DEVICEPLUGINUNIPI_H
