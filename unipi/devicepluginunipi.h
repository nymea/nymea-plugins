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

#include "devices/devicemanager.h"
#include "plugintimer.h"
#include "unipi.h"
#include "neuron.h"
#include "neuronextension.h"

#include <QTimer>
#include <QtSerialBus>
#include <QHostAddress>

class DevicePluginUniPi : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginunipi.json")
    Q_INTERFACES(DevicePlugin)

public:

    explicit DevicePluginUniPi();
    void init() override;

    Device::DeviceError discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    Device::DeviceSetupStatus setupDevice(Device *device) override;
    Device::DeviceError executeAction(Device *device, const Action &action) override;
    void deviceRemoved(Device *device) override;

private:
    UniPi *m_unipi = nullptr;
    QHash<DeviceId, Neuron *> m_neurons;
    QHash<DeviceId, NeuronExtension *> m_neuronExtensions;
    QModbusTcpClient *m_modbusTCPMaster = nullptr;
    QModbusRtuSerialMaster *m_modbusRTUMaster = nullptr;

    QHash<Device *, QTimer *> m_unlatchTimer;
    QTimer *m_reconnectTimer = nullptr;

    bool neuronDeviceInit();
    bool neuronExtensionInterfaceInit();
private slots:
    void onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value);

    void onNeuronDigitalInputStatusChanged(QString &circuit, bool value);
    void onNeuronDigitalOutputStatusChanged(QString &circuit, bool value);
    void onNeuronAnalogInputStatusChanged(QString &circuit, double value);
    void onNeuronAnalogOutputStatusChanged(QString &circuit,double value);
    void onNeuronUserLEDStatusChanged(QString &circuit, bool value);

    void onNeuronExtensionDigitalInputStatusChanged(QString &circuit, bool value);
    void onNeuronExtensionDigitalOutputStatusChanged(QString &circuit, bool value);
    void onNeuronExtensionAnalogInputStatusChanged(QString &circuit, double value);
    void onNeuronExtensionAnalogOutputStatusChanged(QString &circuit,double value);
    void onNeuronExtensionUserLEDStatusChanged(QString &circuit, bool value);

    void onReconnectTimer();

    void onModbusTCPErrorOccurred(QModbusDevice::Error error);
    void onModbusRTUErrorOccurred(QModbusDevice::Error error);
    void onModbusTCPStateChanged(QModbusDevice::State state);
    void onModbusRTUStateChanged(QModbusDevice::State state);

    void onUniPiDigitalInputStatusChanged(const QString &circuit, bool value);
    void onUniPiDigitalOutputStatusChanged(const QString &circuit, bool value);
    void onUniPiAnalogInputStatusChanged(const QString &circuit, double value);
    void onUniPiAnalogOutputStatusChanged(const QString &circuit,double value);
};

#endif // DEVICEPLUGINUNIPI_H
