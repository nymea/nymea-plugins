#include "nymealight.h"

NymeaLight::NymeaLight(WS2812FX *strip) :
    m_strip(strip)
{

}


NymeaLight::NymeaLight(SoftwareSerial *serial, WS2812FX *strip) :
    m_softwareSerial(serial),
    m_strip(strip)
{

}

NymeaLight::NymeaLight(HardwareSerial &serial, WS2812FX *strip) :
    m_hardwareSerial(&serial),
    m_strip(strip)
{

}

WS2812FX *NymeaLight::strip() const
{
    return m_strip;
}

void NymeaLight::init() 
{
    // Get rid of unused warning from lib
    (void)&_modes;

    // Initialize serial port
    if (m_hardwareSerial) {
        m_hardwareSerial->begin(115200);
    } else if (m_softwareSerial) {
        m_softwareSerial->begin(115200);
    }

    // Initialize strip
    m_strip->init();
    if (m_power) {
        m_strip->setBrightness(m_brightness);
    } else {
        m_strip->setBrightness(0);
    }
    m_strip->setSpeed(m_speed);
    m_strip->setColor(0xff, 0xff, 0xff);
    m_strip->setMode(FX_MODE_STATIC);
    m_strip->start(); 

    m_previouseTime = millis();
    sendReadyNotification();
}

void NymeaLight::process() 
{
    // Read incoming data if there is any
    readData();

    //doAnimations();

    // Perform service for WS2812FX
    m_strip->service();
}

void NymeaLight::doAnimations()
{
    // Perform animation steps every 50 ms
    uint32_t currentTimestamp = millis();
    if (m_previouseTime - currentTimestamp > 50000) {
        //debugPrint("Tick");
        m_previouseTime = currentTimestamp;

        if (m_brightnessFadeDuration > 0 && m_brightnessTargetValue != m_strip->getBrightness()) {
            // Calculate step for brightness animation
            if (m_brightnessStartValue < m_brightnessTargetValue) {
                m_strip->setBrightness(m_strip->getBrightness() - 1);
            } else {
                m_strip->setBrightness(m_strip->getBrightness() + 1);
            }
        }
    }
}

void NymeaLight::debugPrint(const char message[])
{
    streamByte(SlipProtocolEnd, true);
    streamByte(NotificationDebugMessage);
    streamByte(m_notificationId);
    m_notificationId++;
    for (size_t i = 0; i < strlen(message); i++) {
        streamByte(message[i]);
    }
    streamByte(SlipProtocolEnd, true);
    flushSerial();
}

void NymeaLight::flushSerial()
{
    if (m_hardwareSerial) {
        m_hardwareSerial->flush();
    } else if (m_softwareSerial) {
        m_softwareSerial->flush();
    }
}

void NymeaLight::sendReadyNotification()
{
    streamByte(SlipProtocolEnd, true);
    streamByte(NotificationReady);
    streamByte(m_notificationId);
    m_notificationId++;
    streamByte(SlipProtocolEnd, true);
    flushSerial();
}

void NymeaLight::readData()
{
    if (m_hardwareSerial) {
        while (m_hardwareSerial->available()) {
            uint8_t receivedByte = m_hardwareSerial->read();
            processReceivedByte(receivedByte);
        }
    } else if (m_softwareSerial) {
        while (m_softwareSerial->available()) {
            uint8_t receivedByte = m_softwareSerial->read();
            processReceivedByte(receivedByte);
        }
    }
}

void NymeaLight::processData(uint8_t buffer[], uint8_t length)
{
    uint8_t command = buffer[0];
    uint8_t requestId = buffer[1];

    switch (command) {
    case CommandGetStatus: {
        if (length != 2) {
            sendResponse(command, requestId, StatusInvalidPlayload);
            return;
        }

        sendResponse(command, requestId, StatusSuccess);
        break;
    }

    case CommandSetPower: {
        if (length != 5) {
            sendResponse(command, requestId, StatusInvalidPlayload);
            return;
        }

        // uint16_t fadeDuration = (buffer[2] << 8 ) | buffer[3];
        uint8_t powerInt = buffer[4];
        if (powerInt == 0) {
            m_power = false;
        } else {
            m_power = true;
        }

        if (m_power) {
            m_strip->setBrightness(m_brightness);
        } else {
            m_strip->setBrightness(0);
        }

        // TODO: animate brightness with fade duration

        sendResponse(command, requestId, StatusSuccess);
        break;
    }

    case CommandSetColor: {
        if (length != 7) {
            sendResponse(command, requestId, StatusInvalidPlayload);
            return;
        }

        //uint32_t currentColor = m_strip->getColor();
        //uint32_t targetColor = ((uint32_t) << 16) | ((uint32_t)buffer[5] << 8) | buffer[6];
        // uint16_t fadeDuration = (buffer[2] << 8 ) | buffer[3];
        uint8_t red = buffer[4];
        uint8_t green = buffer[5];
        uint8_t blue = buffer[6];
        m_strip->setColor(red, green, blue);
        delayMicroseconds(1000);
        sendResponse(command, requestId, StatusSuccess);
        break;
    }

    case CommandSetBrightness: {
        if (length != 5) {
            sendResponse(command, requestId, StatusInvalidPlayload);
            return;
        }

        //uint16_t fadeDuration = (buffer[2] << 8 ) | buffer[3];
        // m_brightnessStartValue = m_strip->getBrightness();

        m_brightness =  buffer[4];
        if (m_power) {
            m_strip->setBrightness(m_brightness);
        }

        delayMicroseconds(1000);
        sendResponse(command, requestId, StatusSuccess);
        break;
    }

    case CommandSetSpeed: {
        if (length != 6) {
            sendResponse(command, requestId, StatusInvalidPlayload);
            return;
        }

        // uint16_t fadeDuration = (buffer[2] << 8 ) | buffer[3];
        uint16_t speed = (buffer[4] << 8 ) | buffer[5];
        m_strip->setSpeed(speed);
        delayMicroseconds(1000);
        sendResponse(command, requestId, StatusSuccess);
        break;
    }

    case CommandSetEffect: {
        if (length != 3) {
            sendResponse(command, requestId, StatusInvalidPlayload);
            return;
        }

        m_strip->setMode(buffer[2]);
        delayMicroseconds(1000);
        sendResponse(command, requestId, StatusSuccess);
        break;
    }

    case CommandCustom:
        
        break;

    default:
        sendResponse(command, requestId, StatusInvalidCommand);
        break;
    }

    m_strip->show();
    m_strip->service();
}

void NymeaLight::sendResponse(uint8_t command, uint8_t requestId, Status status)
{
    streamByte(SlipProtocolEnd, true);
    streamByte(command);
    streamByte(requestId);
    streamByte(status);
    streamByte(SlipProtocolEnd, true);

    flushSerial();
}

void NymeaLight::processReceivedByte(uint8_t receivedByte)
{
    if (m_protocolEscaping) {
        switch (receivedByte) {
        case SlipProtocolTransposedEnd:
            m_buffer[m_bufferIndex++] = SlipProtocolEnd;
            m_protocolEscaping = false;
            break;
        case SlipProtocolTransposedEsc:
            m_buffer[m_bufferIndex++] = SlipProtocolEsc;
            m_protocolEscaping = false;
            break;
        default:
            // SLIP protocol violation...received escape, but it is not an escaped byte
            break;
        }
    }

    switch (receivedByte) {
    case SlipProtocolEnd:
        // We are done with this package, process it and reset the buffer
        if (m_bufferIndex > 0) {
            processData(m_buffer, m_bufferIndex);
        }
        m_bufferIndex = 0;
        m_protocolEscaping = false;
        break;
    case SlipProtocolEsc:
        // The next byte will be escaped, lets wait for it
        m_protocolEscaping = true;
        break;
    default:
        // Nothing special, just add to buffer
        m_buffer[m_bufferIndex++] = receivedByte;
        break;
    }
}

void NymeaLight::streamByte(uint8_t dataByte, boolean specialCharacter)
{
    // If this is a special character, write it without escaping
    if (specialCharacter) {
        writeByte(dataByte);
    } else {
        switch (dataByte) {
        case SlipProtocolEnd:
            writeByte(SlipProtocolEsc);
            writeByte(SlipProtocolTransposedEnd);
            break;
        case SlipProtocolEsc:
            writeByte(SlipProtocolEsc);
            writeByte(SlipProtocolTransposedEsc);
            break;
        default:
            writeByte(dataByte);
            break;
        }
    }
}

void NymeaLight::writeByte(uint8_t dataByte)
{
    if (m_hardwareSerial) {
        m_hardwareSerial->write(dataByte);
    } else if (m_softwareSerial) {
        m_softwareSerial->write(dataByte);
    }
}
