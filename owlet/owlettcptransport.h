#ifndef OWLETTCPTRANSPORT_H
#define OWLETTCPTRANSPORT_H

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>

#include "owlettransport.h"

class OwletTcpTransport : public OwletTransport
{
    Q_OBJECT
public:
    explicit OwletTcpTransport(const QHostAddress &hostAddress, quint16 port, QObject *parent = nullptr);

    bool connected() const override;
    void sendData(const QByteArray &data) override;

public slots:
    void connectTransport() override;
    void disconnectTransport() override;

private:
    QTcpSocket *m_socket = nullptr;
    QHostAddress m_hostAddress;
    quint16 m_port;

};

#endif // OWLETTCPTRANSPORT_H
