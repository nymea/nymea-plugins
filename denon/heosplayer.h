#ifndef HEOSPLAYER_H
#define HEOSPLAYER_H

#include <QObject>

class HeosPlayer : public QObject
{
    Q_OBJECT
public:
    explicit HeosPlayer(int playerId, QObject *parent = 0);
    explicit HeosPlayer(int playerId, QString name, QString serialNumber, QObject *parent = 0);

    QString name();
    void setName(QString name);
    int playerId();
    int groupId();
    void setGroupId(int groupId);
    QString playerModel();
    QString playerVersion();
    QString network();
    QString serialNumber();
    QString lineOut();
    QString control();

private:
    int m_playerId;
    int m_groupId;
    QString m_serialNumber;
    QString m_name;
    QString m_lineOut;
    QString m_network;
    QString m_playerModel;
    QString m_playerVersion;
    QString m_control;


signals:

public slots:
};

#endif // HEOSPLAYER_H
