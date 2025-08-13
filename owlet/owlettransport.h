#ifndef OWLETTRANSPORT_H
#define OWLETTRANSPORT_H

#include <QObject>

class OwletTransport : public QObject
{
    Q_OBJECT
public:
    explicit OwletTransport(QObject *parent = nullptr);

    virtual bool connected() const;
    virtual void sendData(const QByteArray &data) = 0;

public slots:
    virtual void connectTransport() = 0;
    virtual void disconnectTransport() = 0;

signals:
    void connectedChanged(bool connected);
    void dataReceived(const QByteArray &data);
    void error();

protected:
    bool m_connected;

};

#endif // OWLETTRANSPORT_H
