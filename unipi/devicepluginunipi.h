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

    enum NeuronTypes {
        S103,
        M103,
        M203,
        M303,
        M403,
        M503,
        L203,
        L303,
        L403,
        L503,
        L513
    };

    enum ExtensionTypes {
        xS10,
        xS20,
        xS30,
        xS40,
        xS50
    };

    QHash<DimmerSwitch *, Device*> m_dimmerSwitches;
    PluginTimer *m_refreshTimer = nullptr;
    ModbusTCPMaster *m_modbusTCPMaster = nullptr;
    ModbusRTUMaster *m_modbusRTUMaster = nullptr;

    void setDigitalOutput(NeuronTypes neuronType, const QString &circuit, bool value);
    bool getDigitalOutput(NeuronTypes neuronType, const QString &circuit);
    bool getDigitalInput(NeuronTypes neuronType, const QString &circuit);

    void setAnalogOutput(NeuronTypes neuronType, const QString &circuit, double value);
    double getAnalogOutput(NeuronTypes neuronType, const QString &circuit);
    double getAnalogInput(NeuronTypes neuronType, const QString &circuit);

    void setExtensionDigitalOutput(ExtensionTypes extensionType, int slaveAddress, const QString &circuit, bool value);
    bool getExtensionDigitalOutput(ExtensionTypes extensionType, int slaveAddress, const QString &circuit);
    bool getExtensionDigitalInput(ExtensionTypes extensionType, int slaveAddress, const QString &circuit);

    void setExtensionAnalogOutput(ExtensionTypes extensionType, int slaveAddress, const QString &circuit, double value);
    double getExtensionAnalogOutput(ExtensionTypes extensionType, int slaveAddress, const QString &circuit);
    double getExtensionAnalogInput(ExtensionTypes extensionType, int slaveAddress, const QString &circuit);

private slots:
    void onRefreshTimer();

    void onDimmerSwitchPressed();
    void onDimmerSwitchLongPressed();
    void onDimmerSwitchDoublePressed();
    void onDimmerSwitchDimValueChanged(int dimValue);
};

#endif // DEVICEPLUGINUNIPI_H
