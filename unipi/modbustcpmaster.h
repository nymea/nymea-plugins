#ifndef MODBUSTCPMASTER_H
#define MODBUSTCPMASTER_H

#include <QObject>

class ModbusTCPMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusTCPMaster(QObject *parent = nullptr);

signals:

public slots:
};

#endif // MODBUSTCPMASTER_H