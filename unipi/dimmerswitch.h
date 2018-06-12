#ifndef DIMMERSWITCH_H
#define DIMMERSWITCH_H

#include <QObject>
#include <QTimer>

class DimmerSwitch : public QObject
{
    Q_OBJECT
public:
    explicit DimmerSwitch(QObject *parent = 0);
    ~DimmerSwitch();

    void setPower(const bool power);
    bool getPower();
    void setDimValue(const int dimValue);
    int getDimValue();

    QTimer *m_doublePressedTimer = nullptr;
    QTimer *m_longPressedTimer = nullptr;
    QTimer *m_dimmerTimer = nullptr;

private:
    bool m_power;
    int m_dimValue = 0;
    bool m_countingUp = true;
    bool m_powerWasLow = false; //flag to indicate the power was low within the double pressed time frame


signals:
    void pressed();
    void longPressed();
    void doublePressed();
    void dimValueChanged(int dimValue);

private slots:
    void onLongPressedTimeout();
    void onDimmerTimeout();

};

#endif // DIMMERSWITCH_H
