#include "heosplayer.h"

HeosPlayer::HeosPlayer(int playerId, QObject *parent) :
    QObject(parent),
    m_playerId(playerId)
{

}

HeosPlayer::HeosPlayer(int playerId, QString name, QString serialNumber, QObject *parent) :
    QObject(parent),
    m_playerId(playerId),
    m_serialNumber(serialNumber),
    m_name(name)
{

}

QString HeosPlayer::name()
{
    return m_name;
}

void HeosPlayer::setName(QString name)
{
    m_name = name;
}

int HeosPlayer::playerId()
{
    return m_playerId;
}

int HeosPlayer::groupId()
{
    return m_groupId;
}

void HeosPlayer::setGroupId(int groupId)
{
    m_groupId = groupId;
}

QString HeosPlayer::playerModel()
{
    return m_playerModel;
}

QString HeosPlayer::playerVersion()
{
    return m_playerVersion;
}

QString HeosPlayer::network()
{
    return m_network;
}

QString HeosPlayer::serialNumber()
{
    return m_serialNumber;
}

QString HeosPlayer::lineOut()
{
    return m_lineOut;
}

QString HeosPlayer::control()
{
    return m_control;
}


