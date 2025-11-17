// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

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

    float filterValue(float value);

    bool isReady() const;
    void reset();

    Type filterType() const;

    QVector<float> inputData() const;
    QVector<float> outputData() const;

    // Filter configuration
    uint windowSize() const;
    void setFilterWindowSize(uint windowSize = 20);

    float lowPassAlpha() const;
    void setLowPassAlpha(float alpha = 0.2f);

    float highPassAlpha() const;
    void setHighPassAlpha(float alpha = 0.2f);

private:
    Type m_filterType = TypeLowPass;
    uint m_filterWindowSize = 20;
    float m_lowPassAlpha = 0.2f;
    float m_highPassAlpha = 0.2f;

    float m_averageSum = 0;

    QVector<float> m_inputData;
    QVector<float> m_outputData;

    void addInputValue(float value);

    // Filter methods
    float lowPassFilterValue(float value);
    float highPassFilterValue(float value);
    float averageFilterValue(float value);
};

#endif // SENSORFILTER_H
