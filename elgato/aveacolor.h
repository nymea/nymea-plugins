/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016-2018 Simon Stürz <simon.stuerz@guh.io>              *
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

#ifndef AVEACOLOR_H
#define AVEACOLOR_H

#include <QObject>
#include <QColor>
#include <QDebug>

struct ColorRgbw {
  uint red;
  uint green;
  uint blue;
  uint white;
};

class AveaColor
{
public:
    AveaColor(const QColor &color);
    AveaColor(uint white, uint red, uint green, uint blue);

    QColor color() const;

    uint white() const;
    uint red() const;
    uint green() const;
    uint blue() const;

    QByteArray toByteArray();

private:
    QColor m_color;
    uint m_white = 0;
    uint m_red = 0;
    uint m_green = 0;
    uint m_blue = 0;

    uint saturation(ColorRgbw rgbw);
    uint getWhite(ColorRgbw rgbw);
    uint getWhite(ColorRgbw rgbw, int redMax, int greenMax, int blueMax);
    ColorRgbw rgbToRgbw(unsigned int red, unsigned int green, unsigned int blue);
};

QDebug operator<<(QDebug dbg, const AveaColor &color);


#endif // AVEACOLOR_H
