#ifndef INPUTDEVICEEVENTMONITOR_H
#define INPUTDEVICEEVENTMONITOR_H

#include <QObject>

class InputDeviceEventMonitor : public QObject
{
    Q_OBJECT
public:
    explicit InputDeviceEventMonitor(const QString &fileName, QObject *parent = nullptr);

public slots:
    void process();

signals:
    void eventOccurred(quint16 type, quint16 code, qint32 value);
    void finished();

private:
    QString m_fileName;

};

#endif // INPUTDEVICEEVENTMONITOR_H
