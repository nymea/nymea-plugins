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

#include "sensorfilter.h"

#include <QDebug>

SensorFilter::SensorFilter(Type filterType, QObject *parent) :
    QObject(parent),
    m_filterType(filterType)
{

}

float SensorFilter::filterValue(float value)
{
    float resultValue =  value;
    switch (m_filterType) {
    case TypeLowPass:
        resultValue = lowPassFilterValue(value);
        break;
    case TypeHighPass:
        resultValue = highPassFilterValue(value);
        break;
    case TypeAverage:
        resultValue = averageFilterValue(value);
        break;
    default:
        break;
    }

    return resultValue;
}

bool SensorFilter::isReady() const
{
    // Note: filter is ready once 10% of window filled
    return m_inputData.size() >= m_filterWindowSize * 0.1;
}

void SensorFilter::reset()
{
    m_averageSum = 0;
    m_inputData.clear();
}

SensorFilter::Type SensorFilter::filterType() const
{
    return m_filterType;
}

QVector<float> SensorFilter::inputData() const
{
    return m_inputData;
}

QVector<float> SensorFilter::outputData() const
{
    return m_outputData;
}

uint SensorFilter::windowSize() const
{
    return m_filterWindowSize;
}

void SensorFilter::setFilterWindowSize(uint windowSize)
{
    Q_ASSERT_X(windowSize > 0, "value out of range", "The filter window size must be bigger than 0");
    m_filterWindowSize = windowSize;
}

float SensorFilter::lowPassAlpha() const
{
    return m_lowPassAlpha;
}

void SensorFilter::setLowPassAlpha(float alpha)
{
    Q_ASSERT_X(alpha > 0 && alpha <= 1, "value out of range", "The alpha low pass filter value must be [ 0 < alpha <= 1 ]");
    m_lowPassAlpha = alpha;
}

float SensorFilter::highPassAlpha() const
{
    return m_highPassAlpha;
}

void SensorFilter::setHighPassAlpha(float alpha)
{
    Q_ASSERT_X(alpha > 0 && alpha <= 1, "value out of range", "The alpha high pass filter value must be [ 0 < alpha <= 1 ]");
    m_highPassAlpha = alpha;
}

void SensorFilter::addInputValue(float value)
{
    m_inputData.append(value);
    if (static_cast<uint>(m_inputData.size()) > m_filterWindowSize) {
        m_inputData.removeFirst();
    }
}

float SensorFilter::lowPassFilterValue(float value)
{
    addInputValue(value);

    // Check if we have enough data for filtering
    if (m_inputData.size() < 2) {
        return value;
    }

    QVector<float> outputData;
    outputData.append(m_inputData.at(0));
    for (int i = 1; i < m_inputData.size(); i++) {
        // y[i] := y[i-1] + α * (x[i] - y[i-1])
        outputData.append(outputData.at(i - 1) + m_lowPassAlpha * (m_inputData.at(i) - outputData.at(i - 1)));
    }

    m_outputData = outputData;

    return m_outputData.last();
}

float SensorFilter::highPassFilterValue(float value)
{
    addInputValue(value);

    // Check if we have enough data for filtering
    if (m_inputData.size() < 2) {
        return value;
    }

    QVector<float> outputData;
    outputData.append(m_inputData.at(0));
    for (int i = 1; i < m_inputData.size(); i++) {
        // y[i] := α * y[i-1] + α * (x[i] - x[i-1])
        outputData.append(m_highPassAlpha * outputData.at(i - 1) + m_highPassAlpha * (m_inputData.at(i) - m_inputData.at(i - 1)));
    }

    m_outputData = outputData;
    return m_outputData.last();
}

float SensorFilter::averageFilterValue(float value)
{
    if (m_inputData.isEmpty()) {
        addInputValue(value);
        m_averageSum = value;
        return value;
    }

    if (static_cast<uint>(m_inputData.size()) >= m_filterWindowSize) {
        m_averageSum -= m_inputData.takeFirst();
    }

    addInputValue(value);
    m_averageSum += value;
    return m_averageSum / m_inputData.size();
}
