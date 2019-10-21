#ifndef EQIVABLUETOOTH_H
#define EQIVABLUETOOTH_H

#include <QObject>

#include "hardware/bluetoothlowenergy/bluetoothlowenergymanager.h"


class EqivaBluetooth : public QObject
{
    Q_OBJECT
public:
    enum Mode {
        ModeAuto,
        ModeManual,
        ModeHoliday
    };
    explicit EqivaBluetooth(BluetoothLowEnergyManager *bluetoothManager, const QBluetoothAddress &hostAddress, const QString &name, QObject *parent = nullptr);
    ~EqivaBluetooth();
    void setName(const QString &name);

    bool available() const;

    bool enabled() const;
    int setEnabled(bool enabled);

    bool locked() const;
    int setLocked(bool locked);

    bool boostEnabled() const;
    int setBoostEnabled(bool enabled);

    qreal targetTemperature() const;
    int setTargetTemperature(qreal targetTemperature);

    Mode mode() const;
    int setMode(Mode mode);

    bool windowOpen() const;

    quint8 valveOpen() const;

    bool batteryCritical() const;

signals:
    void availableChanged();
    void enabledChanged();
    void lockedChanged();
    void boostEnabledChanged();
    void modeChanged();
    void windowOpenChanged();
    void targetTemperatureChanged();
    void valveOpenChanged();
    void batteryCriticalChanged();

    void commandResult(int id, bool result);

private slots:
    void controllerStateChanged(const QLowEnergyController::ControllerState &state);
    void serviceStateChanged(QLowEnergyService::ServiceState newState);
    void characteristicChanged(const QLowEnergyCharacteristic &info, const QByteArray &value);

    void writeCharacteristic(const QBluetoothUuid &characteristicUuid, const QByteArray &data);

    void sendDate();

    // Name parameter used for debugging purposes
    int enqueue(const QString &name, const QByteArray &data);
    void processCommandQueue();

private:
    BluetoothLowEnergyManager* m_bluetoothManager = nullptr;
    BluetoothLowEnergyDevice* m_bluetoothDevice = nullptr;
    QLowEnergyService *m_eqivaService = nullptr;
    QTimer m_refreshTimer;

    QString m_name;

    bool m_available = false;
    bool m_enabled = false;
    bool m_locked = false;
    bool m_boostEnabled = false;
    qreal m_targetTemp = 0;
    qreal m_cachedTargetTemp = 0;
    Mode m_mode = ModeAuto;
    bool m_windowOpen = false;
    quint8 m_valveOpen = 0;
    bool m_batteryCritical = false;

    QTimer m_reconnectTimer;
    int m_reconnectAttempt = 0;

    struct Command {
        QString name; // For debug prints
        QByteArray data;
        qint32 id = -1;
    };
    QList<Command> m_commandQueue;
    Command m_currentCommand;
    QTimer m_commandTimeout;

    quint16 m_nextCommandId = 0;
};

class EqivaBluetoothDiscovery: public QObject
{
    Q_OBJECT
public:
    EqivaBluetoothDiscovery(BluetoothLowEnergyManager *bluetoothManager, QObject *parent = nullptr);

    bool startDiscovery();

private slots:
    void deviceDiscoveryDone();

signals:
    void finished(const QStringList &results);

private:
    BluetoothLowEnergyManager *m_bluetoothManager = nullptr;

};

#endif // EQIVABLUETOOTH_H
