/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef ADS1115_H
#define ADS1115_H

#include <QThread>
#include <QMutex>

#include <hardware/i2c/i2cdevice.h>

class ADS1115Channel: public I2CDevice
{
    Q_OBJECT
public:

    enum Gain {
        Gain_6_144 = 0,
        Gain_4_096 = 1,
        Gain_2_048 = 2,
        Gain_1_024 = 3,
        Gain_0_512 = 4,
        Gain_0_256 = 5
    };

    explicit ADS1115Channel(const QString &portName, int address, int channel, Gain gain, QObject *parent = nullptr);

    QByteArray readData(int fd) override;

private:
    int m_channel = 0;
    Gain m_gain = Gain_4_096;

};

#endif // ADS1115_H
