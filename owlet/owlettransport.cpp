#include "owlettransport.h"

OwletTransport::OwletTransport(QObject *parent) : QObject(parent)
{

}

bool OwletTransport::connected() const
{
    return m_connected;
}
