#include "anelpanel.h"

AnelPanel::AnelPanel(const QHostAddress &hostAddress, QObject *parent) : QObject(parent)
{
    Q_UNUSED(hostAddress)
}
