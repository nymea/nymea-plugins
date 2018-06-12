#include "dimmerswitch.h"

DimmerSwitch::DimmerSwitch(QObject *parent) : QObject(parent)
{
    m_longPressedTimer = new QTimer(this);
    m_longPressedTimer->setSingleShot(true);
    connect(m_longPressedTimer, SIGNAL(timeout()), this, SLOT(onLongPressedTimeout()));

    m_doublePressedTimer = new QTimer(this);
    m_doublePressedTimer->setSingleShot(true);

    m_dimmerTimer = new QTimer(this);
    connect(m_dimmerTimer, SIGNAL(timeout()), this, SLOT(onDimmerTimeout()));
}

DimmerSwitch::~DimmerSwitch()
{
    m_longPressedTimer->deleteLater();
    m_doublePressedTimer->deleteLater();
    m_dimmerTimer->deleteLater();
}

void DimmerSwitch::setPower(const bool power)
{
    if (m_power == power) {
        return;
    }

    m_power = power;
    if(power){
        m_dimmerTimer->start(150);
        m_longPressedTimer->start(2000);

        if (m_doublePressedTimer->isActive()) {
            m_doublePressedTimer->stop();
            emit doublePressed();
        } else {
            m_doublePressedTimer->start(1000);
            emit pressed();
        }

    } else {
        m_dimmerTimer->stop();
        m_longPressedTimer->stop();
    }
}

bool DimmerSwitch::getPower()
{
    return m_power;
}

void DimmerSwitch::setDimValue(const int dimValue)
{
    m_dimValue = dimValue;
    emit dimValueChanged(m_dimValue);
}

int DimmerSwitch::getDimValue()
{
    return m_dimValue;
}

void DimmerSwitch::onDimmerTimeout()
{
    if (m_countingUp) {
        m_dimValue += 3;
        if(m_dimValue >= 100) {
            m_dimValue = 100;
            m_countingUp = false;
        }
    } else {
        m_dimValue -= 3;
        if(m_dimValue <= 0) {
            m_dimValue = 0;
            m_countingUp = true;
        }
    }
    emit dimValueChanged(m_dimValue);
}

void DimmerSwitch::onLongPressedTimeout()
{
    emit longPressed();
}


