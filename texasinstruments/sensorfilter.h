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
