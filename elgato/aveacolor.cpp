#include "aveacolor.h"

AveaColor::AveaColor(const QColor &color)
{
    // Convert rgb to wrgb

    ColorRgbw rgbw = rgbToRgbw(color.red(), color.green(), color.blue());
    Q_UNUSED(rgbw)

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
