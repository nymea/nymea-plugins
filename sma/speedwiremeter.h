/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2021, nymea GmbH
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

#ifndef SPEEDWIREMETER_H
#define SPEEDWIREMETER_H

#include <QObject>
#include <QDateTime>
#include <QTimer>

#include "speedwireinterface.h"

class SpeedwireMeter : public QObject
{
    Q_OBJECT
public:
    explicit SpeedwireMeter(const QHostAddress &address, quint16 modelId, quint32 serialNumber, QObject *parent = nullptr);

    bool initialize();
    bool initialized() const;

    bool reachable() const;

    double currentPower() const;
    double totalEnergyProduced() const;
    double totalEnergyConsumed() const;

    double energyConsumedPhaseA() const;
    double energyConsumedPhaseB() const;
    double energyConsumedPhaseC() const;

    double energyProducedPhaseA() const;
    double energyProducedPhaseB() const;
    double energyProducedPhaseC() const;

    double currentPowerPhaseA() const;
    double currentPowerPhaseB() const;
    double currentPowerPhaseC() const;

    double voltagePhaseA() const;
    double voltagePhaseB() const;
    double voltagePhaseC() const;

    double amperePhaseA() const;
    double amperePhaseB() const;
    double amperePhaseC() const;

    QString softwareVersion() const;


signals:
    void reachableChanged(bool reachable);
    void valuesUpdated();

private:
    SpeedwireInterface *m_interface = nullptr;
    QHostAddress m_address;
    bool m_initialized = false;
    quint16 m_modelId = 0;
    quint32 m_serialNumber = 0;

    QTimer m_timer;
    bool m_reachable = false;
    qint64 m_lastSeenTimestamp = 0;

    double m_currentPower = 0;
    double m_totalEnergyProduced = 0;
    double m_totalEnergyConsumed = 0;

    double m_energyConsumedPhaseA = 0;
    double m_energyConsumedPhaseB = 0;
    double m_energyConsumedPhaseC = 0;

    double m_energyProducedPhaseA = 0;
    double m_energyProducedPhaseB = 0;
    double m_energyProducedPhaseC = 0;

    double m_currentPowerPhaseA = 0;
    double m_currentPowerPhaseB = 0;
    double m_currentPowerPhaseC = 0;

    double m_voltagePhaseA = 0;
    double m_voltagePhaseB = 0;
    double m_voltagePhaseC = 0;

    double m_amperePhaseA = 0;
    double m_amperePhaseB = 0;
    double m_amperePhaseC = 0;

    QString m_softwareVersion;


private slots:
    void evaluateReachable();
    void processData(const QByteArray &data);

};

#endif // SPEEDWIREMETER_H
