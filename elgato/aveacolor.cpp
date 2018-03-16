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

#include "extern-plugininfo.h"

AveaColor::AveaColor(const QColor &color):
    m_color(color)
{
    // Convert rgb to wrgb

    qCDebug(dcElgato()) << "Convert color" << m_color.red() << m_color.green() << m_color.blue();

    ColorRgbw rgbw = rgbToRgbw(m_color.red(), m_color.green(), m_color.blue());
    m_white = rgbw.white;
    m_red = rgbw.red;
    m_green = rgbw.green;
    m_blue = rgbw.blue;
}

AveaColor::AveaColor(uint white, uint red, uint green, uint blue):
    m_white(white),
    m_red(red),
    m_green(green),
    m_blue(blue)
{
    // Convert rgbw to rgb

    // TODO


}

QColor AveaColor::color() const
{
    return m_color;
}

uint AveaColor::white() const
{
    return m_white;
}

uint AveaColor::red() const
{
    return m_red;
}

uint AveaColor::green() const
{
    return m_green;
}

uint AveaColor::blue() const
{
    return m_blue;
}

QByteArray AveaColor::toByteArray()
{
    return QByteArray();
}

// The saturation is the colorfulness of a color relative to its own brightness.
uint AveaColor::saturation(ColorRgbw rgbw)
{
    // Find the smallest of all three parameters.
    float low = qMin(rgbw.red, qMin(rgbw.green, rgbw.blue));
    // Find the highest of all three parameters.
    float high = qMax(rgbw.red, qMax(rgbw.green, rgbw.blue));
    // The difference between the last two variables
    // divided by the highest is the saturation.
    return qRound(100 * ((high - low) / high));
}

uint AveaColor::getWhite(ColorRgbw rgbw)
{
    return (255 - saturation(rgbw)) / 255 * (rgbw.red + rgbw.green + rgbw.blue) / 3;
}

uint AveaColor::getWhite(ColorRgbw rgbw, int redMax, int greenMax, int blueMax)
{
    // Set the maximum value for all colors.
    rgbw.red = (float)rgbw.red / 255.0 * (float)redMax;
    rgbw.green = (float)rgbw.green / 255.0 * (float)greenMax;
    rgbw.blue = (float)rgbw.blue / 255.0 * (float)blueMax;
    return (255 - saturation(rgbw)) / 255 * (rgbw.red + rgbw.green + rgbw.blue) / 3;
    return 0;
}

ColorRgbw AveaColor::rgbToRgbw(uint red, uint green, uint blue)
{
    uint white = 0;
    ColorRgbw rgbw = {red, green, blue, white};
    rgbw.white = getWhite(rgbw);
    return rgbw;
}

QDebug operator<<(QDebug dbg, const AveaColor &color)
{
    dbg.nospace() << "AveaColor(" << color.color() << " | R:" << color.red() << " G:" << color.green() << "B:" << color.blue() << "W:" << color.white() << ")";
    return dbg;
}
