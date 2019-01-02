#ifndef ANELPANEL_H
#define ANELPANEL_H

#include <QObject>
#include <QHostAddress>
#include <QUdpSocket>

class AnelPanel : public QObject
{
    Q_OBJECT
public:
    explicit AnelPanel(const QHostAddress &hostAddress, QObject *parent = nullptr);

signals:

public slots:
//    QUdpSocket *m_receiveSocket = nullptr;
//    QUdpSocket *m_
};

#endif // ANELPANEL_H
