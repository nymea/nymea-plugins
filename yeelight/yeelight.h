/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
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

#ifndef YEELIGHT_H
#define YEELIGHT_H

#include <QObject>
#include <QTimer>
#include <QHostAddress>
#include <QTcpSocket>

#include "network/networkaccessmanager.h"
#include "devices/device.h"

#include <QRgb>

class Yeelight : public QObject
{
    Q_OBJECT
public:
    enum ColorMode {
        RGB = 1,
        Temperature,
        HSV
    };

    enum Property {
        Power,      //on: smart LED is turned on / off: smart LED is turned off
        Bright,     //Brightness percentage. Range 1 ~ 100
        Ct,         //Color temperature. Range 1700 ~ 6500(k)
        Rgb,        //Color. Range 1 ~ 16777215
        Hue,        //Hue. Range 0 ~ 359
        Sat,        //Saturation. Range 0 ~ 100
        ColorMode,  //1: rgb mode / 2: color temperature mode / 3: hsv mode
        Flowing,    //0: no flow is running / 1:color flow is running
        DelayOff,   //The remaining time of a sleep timer. Range 1 ~ 60 (minutes)
        FlowParams, //Current flow parameters (only meaningful when 'flowing' is 1)
        MusicOn,    //1: Music mode is on / 0: Music mode is off
        Name,       //The name of the device set by “set_name” command
        BgPower,    //Background light power status
        BgFlowing,  //Background light is flowing
        BgFlowParams, //Current flow parameters of background light
        BgCt,       //Color temperature of background light
        BgLmode,    //1: rgb mode / 2: color temperature mode / 3: hsv mode
        BgBright,   //Brightness percentage of background light
        BgRgb,      //Color of background light
        BgHue,      //Hue of background light
        BgSat,      //Saturation of background light
        NlBr,       //Brightness of night mode light
        ActiveMode  //0: daylight mode / 1: moonlight mode (ceiling light only)
    };

    explicit Yeelight(NetworkAccessManager *networkManager, const QHostAddress &address,  quint16 port = 55443, QObject *parent = nullptr);
    bool isConnected();
    void connectDevice();

    int getParam(QList<Property> properties);
    int setName(const QString &name);
    int setColorTemperature(int mirad, int msFadeTime=500);
    int setRgb(QRgb color, int msFadeTime = 500);
    int setBrightness(int percentage, int msFadeTime = 500);
    int setPower(bool power, int msFadeTime = 500);
    int setDefault();
    int startColorFlow();
    int stopColorFlow();
    int flash();
    int flash15s();

private:
    QTcpSocket *m_socket = nullptr;
    QHostAddress m_address;
    quint16 m_port;
    NetworkAccessManager *m_networkManager = nullptr;

private slots:
    void onStateChanged(QAbstractSocket::SocketState state);
    void onReadyRead();

signals:
    void connectionChanged(bool connected);
    void requestExecuted(int requestId, bool success);
    void propertyListReceived(QVariantList value);

    /*
     * Whenever there is state change of smart LED, it will send a notification message
     * to all connected 3rd party devices. This is to make sure all 3rd party devices
     * will get the latest state of the smart LED in time without having to poll the status
     * from time to time.
     */
    void notificationReceived(Property property, QVariant value);
    void powerNotificationReceived(bool status);
    void brightnessNotificationReceived(int percentage);
    void colorTemperatureNotificationReceived(int kelvin);
    void rgbNotificationReceived(int rgbColor);
    void hueNotificationReceived(int hueColor);
    void nameNotificationReceived(const QString &name);
    void saturationNotificationReceived(int percentage);
    //void colorModeNotificationReceived(ColorMode colorMode);
};
#endif // YEELIGHT_H
