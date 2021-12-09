#ifndef SPEEDWIREMETER_H
#define SPEEDWIREMETER_H

#include <QObject>

#include "speedwireinterface.h"

class SpeedwireMeter : public QObject
{
    Q_OBJECT
public:
    explicit SpeedwireMeter(const QHostAddress &address, quint16 modelId, quint32 serialNumber, QObject *parent = nullptr);

    bool initialize();
    bool initialized() const;

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
    void valuesUpdated();

private:
    SpeedwireInterface *m_interface = nullptr;
    QHostAddress m_address;
    bool m_initialized = false;
    quint16 m_modelId = 0;
    quint32 m_serialNumber = 0;

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
    void processData(const QByteArray &data);

};

#endif // SPEEDWIREMETER_H
