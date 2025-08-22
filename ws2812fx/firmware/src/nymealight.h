
/*  
    SLIP transfere: https://tools.ietf.org/html/rfc1055

    Request package format:
    --------------------------
        uint8   : command (< 0xF0)
        uint8   : requestId
        uint8[] : payload (dynamic size, max: 253)


    Response format:
    --------------------------
        uint8   : command (< 0xF0)
        uint8   : requestId (same as request)
        uint8   : status
        uint8[] : payload (optional, max: 253)


    Notification package format:
    --------------------------
        uint8   : command (>= 0xF0)
        uint8   : notificationId
        uint8[] : payload (dynamic size, max: 253)


    Commands:
    --------------------------
        0x00: get ready status 
            payload: empty


        0x01: set power
            payload:
                uint16 : fade duration in ms
                uint8  : power (0x00 = off, 0x01 = on)


        0x02: set color 
            payload:
                uint16 : fade duration in ms
                uint8  : red
                uint8  : green
                uint8  : blue


        0x03: set brightness
            payload:
                uint16 : fade duration in ms
                uint8  : brightness (0-255)


        0x04: set speed
            payload:
                uint16 : fade duration in ms
                uint16 : speed in ms (big endian)


        0x05: set effect
            payload:
                uint8  : effect, see effect list from WS2812FX


        0xef: custom command 
            The payload can be defined free and custom functionality can be implemented


    Notifications:
    --------------------------
        0xF0: Status ready notification 
            payload: empty
                

        0xF1: Debug message notification 
            payload: debug message characters
                

*/

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <WS2812FX.h>

class NymeaLight 
{
public:
    enum Command {
        CommandGetStatus = 0x00,
        CommandSetPower = 0x01,
        CommandSetColor = 0x02,
        CommandSetBrightness = 0x03,
        CommandSetSpeed = 0x04,
        CommandSetEffect = 0x05,
        CommandCustom = 0xEF
    };

    enum Notification {
        NotificationReady = 0xF0,
        NotificationDebugMessage = 0xF1
    };

    enum Status {
        StatusSuccess = 0x00,
        StatusInvalidProtocol = 0x01,
        StatusInvalidCommand = 0x02,
        StatusInvalidPlayload = 0x03,
        StatusUnknownError = 0xff
    };

    NymeaLight(WS2812FX *strip);
    NymeaLight(SoftwareSerial *serial, WS2812FX *strip);
    NymeaLight(HardwareSerial &serial, WS2812FX *strip);

    WS2812FX *strip() const;

    void init();
    void process();

private:
    enum SlipProtocol {
        SlipProtocolEnd = 0xC0,
        SlipProtocolEsc = 0xDB,
        SlipProtocolTransposedEnd = 0xDC,
        SlipProtocolTransposedEsc = 0xDD
    };

    HardwareSerial *m_hardwareSerial = nullptr;
    SoftwareSerial *m_softwareSerial = nullptr;
    WS2812FX *m_strip = nullptr;
    uint8_t m_notificationId = 0;

    // Light states
    bool m_power = false;
    uint8_t m_brightness = 0xff;
    uint32_t m_color = 0x00ffffff;
    uint8_t m_effect = FX_MODE_STATIC;
    uint16_t m_speed = 2000;

    // UART read
    uint8_t m_buffer[255];
    uint8_t m_bufferIndex = 0;
    boolean m_protocolEscaping = false;

    // Animations
    uint32_t m_previouseTime = 0;

    // Brightness animation
    uint32_t m_brightnessProgress = 0; // us
    uint8_t m_brightnessStartValue = 0;
    uint8_t m_brightnessTargetValue = 0;
    uint8_t m_brightnessFadeDuration = 0; //ms

    void doAnimations();
    void debugPrint(const char message[]);
    void flushSerial();
    void sendReadyNotification();

protected:
    virtual void readData();
    virtual void processReceivedByte(uint8_t receivedByte);
    virtual void processData(uint8_t buffer[], uint8_t length);
    virtual void sendResponse(uint8_t command, uint8_t requestId, Status status);
    virtual void streamByte(uint8_t dataByte, boolean specialCharacter = false);
    virtual void writeByte(uint8_t dataByte);

};
