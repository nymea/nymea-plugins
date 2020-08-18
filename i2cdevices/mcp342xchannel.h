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

#ifndef MCP342X_H
#define MCP342X_H

#include <QThread>
#include <QMutex>

#include <hardware/i2c/i2cdevice.h>

class MCP342XChannel: public I2CDevice
{
    Q_OBJECT
public:

    enum Gain {
        Gain_1 = 0,
        Gain_2 = 1,
        Gain_4 = 2,
        Gain_8 = 3
    };

    enum ConfRegisterBits {
        G0 = 0, // Gain Selection
        G1, // Gain Selection
        S0, // Sample Rate
        S1, // Sample Rate
        OC, // Conversion Mode Bit
        C0, // Channel Selection
        C1, // Channel Selection
        RDY // Ready Bit
    };

    enum SampleRateSelectionBit {
        Bits12 = 0,
        Bits14 = 1,
        Bits16 = 2,
        Bits18 = 3
    };

    explicit MCP342XChannel(const QString &portName, int address, int channel, Gain gain, QObject *parent = nullptr);

    QByteArray readData(int fd) override;

private:
    int m_channel = 0;
    Gain m_gain = Gain_1;
};

#endif // MCP342X_H
