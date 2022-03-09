#ifndef SERIALPORTMONITOR_H
#define SERIALPORTMONITOR_H

#include <QHash>
#include <QObject>
#include <QSerialPortInfo>
#include <QSocketNotifier>

#include <libudev.h>

class SerialPortMonitor : public QObject
{
    Q_OBJECT
public:
    typedef struct SerialPortInfo {
        QString manufacturer;
        QString product;
        QString serialNumber;
        QString systemLocation;
        quint16 vendorId;
        quint16 productId;
    } SerialPortInfo;

    explicit SerialPortMonitor(QObject *parent = nullptr);
    ~SerialPortMonitor();

    QList<SerialPortInfo> serialPortInfos() const;

signals:
    void serialPortAdded(const SerialPortInfo &serialPortInfo);
    void serialPortRemoved(const SerialPortInfo &serialPortInfo);

private:
    struct udev *m_udev = nullptr;
    struct udev_monitor *m_monitor = nullptr;
    QSocketNotifier *m_notifier = nullptr;

    QHash<QString, SerialPortInfo> m_serialPortInfos;

};

QDebug operator<< (QDebug dbg, const SerialPortMonitor::SerialPortInfo &serialPortInfo);


#endif // SERIALPORTMONITOR_H
