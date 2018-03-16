#ifndef AVEACOLOR_H
#define AVEACOLOR_H

#include <QObject>
#include <QColor>

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

    uint white() const;
    uint red() const;
    uint green() const;
    uint blue() const;

    QByteArray toByteArray();

private:
    uint m_white = 0;
    uint m_red = 0;
    uint m_green = 0;
    uint m_blue = 0;

    uint saturation(ColorRgbw rgbw);
    uint getWhite(ColorRgbw rgbw);
    uint getWhite(ColorRgbw rgbw, int redMax, int greenMax, int blueMax);
    ColorRgbw rgbToRgbw(unsigned int red, unsigned int green, unsigned int blue);
};

#endif // AVEACOLOR_H
