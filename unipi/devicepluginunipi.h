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
#include "neuron.h"
#include "neuronextension.h"
#include "modbusrtumaster.h"
#include "modbustcpmaster.h"

#include <QTimer>

class DevicePluginUniPi : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginunipi.json")
    Q_INTERFACES(DevicePlugin)

public:

    explicit DevicePluginUniPi();
    void init() override;

    DeviceManager::DeviceSetupStatus setupDevice(Device *device) override;
    void postSetupDevice(Device *device) override;
    DeviceManager::DeviceError executeAction(Device *device, const Action &action) override;
    void deviceRemoved(Device *device) override;

private:

    QHash<DeviceId, Neuron *> m_neurons;
    QHash<DeviceId, NeuronExtension *> m_neuronExtensions;
    QHash<DimmerSwitch *, Device*> m_dimmerSwitches;
    ModbusTCPMaster *m_modbusTCPMaster = nullptr;
    ModbusRTUMaster *m_modbusRTUMaster = nullptr;

    QHash<Device *, QTimer *> m_unlatchTimer;

private slots:
    void onDimmerSwitchPressed();
    void onDimmerSwitchLongPressed();
    void onDimmerSwitchDoublePressed();
    void onDimmerSwitchDimValueChanged(int dimValue);

    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);

    void onDigitalInputStatusChanged(QString &circuit, bool value);
    void onDigitalOutputStatusChanged(QString &circuit, bool value);
};

#endif // DEVICEPLUGINUNIPI_H
