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

#include "ws281xspicontroller.h"
#include "extern-plugininfo.h"

#include <QDebug>

#include <unistd.h>

#define GPIO_PIN                10
#define DMA                     10
#define TARGET_FREQ             WS2811_TARGET_FREQ

#define STRIP_TYPE              WS2811_STRIP_GRB

Ws281xSpiController::Ws281xSpiController()
{

}

Ws281xSpiController::~Ws281xSpiController()
{
    freeLEDString();
}

bool Ws281xSpiController::setBrightness(int brightness)
{
    bool result = true;

    qCDebug(dcWs281xspi()) << "setBrightness in controller called";
    m_brightness = brightness;

    if ((brightness >= WS281X_MIN_BRIGHTNESS) && 
            (brightness <= WS281X_MAX_BRIGHTNESS)) {
        result = update();
    }

    // emit signal only in case of success
    if (result == true) {
        emit brightnessChanged(brightness);
    }

    return result;
}

bool Ws281xSpiController::setColor(QColor color)
{
    bool result = true;

    qCDebug(dcWs281xspi()) << "setColor in controller called";
    m_color = color;

    result = update();

    // emit signal only in case of success
    if (result == true) {
        emit colorChanged(color);
    }

    return result;
}

bool Ws281xSpiController::setLength(int length)
{
    bool result = true;

    qCDebug(dcWs281xspi()) << "setLength in controller called: " << length;

    if (length > 0) {
        m_length = length;

        result = update();
    } else {
        result = false;
    }

    // emit signal only in case of success
    if (result == true) {
        emit lengthChanged(length);
    }

    return result;
}

bool Ws281xSpiController::setPower(bool power)
{
    bool result = true;

    qCDebug(dcWs281xspi()) << "setPower in controller called: " << power;
    m_power = power;

    result = update();

    // emit signal only in case of success
    if (result == true) {
        emit powerChanged(power);
    }

    return result;
}

bool Ws281xSpiController::setSPIPin(int spiPin)
{
    bool result = true;

    qCDebug(dcWs281xspi()) << "setSPIpin in controller called";

    // at the moment only pin 10 is accepted since this is the only
    // SPI pin that is available on all Raspberry Pi models.
    // to be extended in future versions
    if (spiPin == 10) {
        m_spiPin = spiPin;

        result = update();
    } else {
        result = false;
    }

    return result;
}

void Ws281xSpiController::freeLEDString()
{
    ws2811_fini(&m_ledstring);
}

bool Ws281xSpiController::initLEDString()
{
    bool result = true;

    m_ledstring.freq = TARGET_FREQ;
    m_ledstring.dmanum = DMA;
    m_ledstring.channel[0].gpionum = m_spiPin;
    m_ledstring.channel[0].invert = 0;
    m_ledstring.channel[0].count = m_length;
    m_ledstring.channel[0].strip_type = STRIP_TYPE;
    m_ledstring.channel[1].gpionum = 0;
    m_ledstring.channel[1].invert = 0;
    m_ledstring.channel[1].count = 0;
    m_ledstring.channel[1].brightness = 0;

    qCDebug(dcWs281xspi()) << "trying to allocate memory for " << m_length << " LEDs";

    ws2811_return_t lib_result;
    
    lib_result = ws2811_init(&m_ledstring);

    if (lib_result) {
        qCWarning(dcWs281xspi()) << "ws2811_init returned error code: " << result;
        result = false;
    }

    return result;
}

bool Ws281xSpiController::update()
{
    bool result = true;
    ws2811_led_t ledColor{};

    // convert QColor into ws2811_led_t
    ledColor |= m_color.red() << 16;
    ledColor |= m_color.green() << 8;
    ledColor |= m_color.blue();

    result = initLEDString();

    if (m_power == true) {
        m_ledstring.channel[0].brightness = m_brightness;
    } else {
        m_ledstring.channel[0].brightness = 0;
    }

    for (int i = 0; i < (m_length); i++) {
        m_ledstring.channel[0].leds[i] = ledColor;
    }

    qCDebug(dcWs281xspi()) << "LED strip initialized with result: " << result;

    if (result) {
        ws2811_render(&m_ledstring);
    }

    return result;
}

