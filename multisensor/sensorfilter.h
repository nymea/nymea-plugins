/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015-2018 Simon Stuerz <simon.stuerz@guh.io>             *
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

#ifndef SENSORFILTER_H
#define SENSORFILTER_H

#include <QObject>
#include <QVector>

class SensorFilter : public QObject
{
    Q_OBJECT
public:
    enum Type {
        TypeLowPass,
        TypeHighPass,
        TypeAverage
    };
    Q_ENUM(Type)

    explicit SensorFilter(Type filterType, QObject *parent = nullptr);

    double filterValue(double value);

    bool isReady() const;
    void reset();

    Type filterType() const;

    QVector<double> inputData() const;
    QVector<double> outputData() const;

    // Filter configuration
    uint windowSize() const;
    void setFilterWindowSize(uint windowSize = 20);

    double lowPassAlpha() const;
    void setLowPassAlpha(double alpha = 0.2);

    double highPassAlpha() const;
    void setHighPassAlpha(double alpha = 0.2);

private:
    Type m_filterType = TypeLowPass;
    int m_filterWindowSize = 20;
    double m_lowPassAlpha = 0.2;
    double m_highPassAlpha = 0.2;

    double m_averageSum = 0;

    QVector<double> m_inputData;
    QVector<double> m_outputData;

    void addInputValue(double value);

    // Filter methods
    double lowPassFilterValue(double value);
    double highPassFilterValue(double value);
    double averageFilterValue(double value);
};

#endif // SENSORFILTER_H
