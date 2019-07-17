/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernbard Trinnes <bernhard.trinnes@nymea.io         *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef MCP3422_H
#define MCP3422_H

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QMutexLocker>

class MCP3422 : public QThread
{
    Q_OBJECT
public:
    explicit MCP3422(const QString &i2cPortName, int i2cAddress = 0x48, QObject *parent = nullptr);
    ~MCP3422() override;

protected:
    void run() override;

private:
    QString m_i2cPortName;
    int m_i2cAddress;

    QMutex m_stopMutex;
    bool m_stop = false;
    QMutex m_valueMutex;
    int m_fileDescriptor = -1;

public slots:
    bool enable();
    void disable();
};

#endif // MCP3422_H
