/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by copyright law, and
* remains the property of nymea GmbH. All rights, including reproduction, publication,
* editing and translation, are reserved. The use of this project is subject to the terms of a
* license agreement to be concluded with nymea GmbH in accordance with the terms
* of use of nymea GmbH, available under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* This project may also contain libraries licensed under the open source software license GNU GPL v.3.
* Alternatively, this project may be redistributed and/or modified under the terms of the GNU
* Lesser General Public License as published by the Free Software Foundation; version 3.
* this project is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License along with this project.
* If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under contact@nymea.io
* or see our FAQ/Licensing Information on https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef NANOLEAF_H
#define NANOLEAF_H

#include <QObject>
#include <QTimer>
#include <QUuid>
#include <QHostAddress>
#include <QColor>

#include "network/networkaccessmanager.h"
#include "devices/device.h"

class Nanoleaf : public QObject
{
    Q_OBJECT
public:
    struct ControllerInfo {
      QString name;
      QString serialNumber;
      QString manufacturer;
      QString firmwareVersion;
      QString model;
    };

    enum GestureID {
        SingleTap   = 0,
        DoubleTap   = 1,
        SwipeUp     = 2,
        SwipeDown 	= 3,
        SwipeLeft   = 4,
        SwipeRight  = 5
    };

    explicit Nanoleaf(NetworkAccessManager *networkManager, const QHostAddress &address, int port = 16021, QObject *parent = nullptr);
    void setIpAddress(const QHostAddress &address);
    QHostAddress ipAddress();

    void setPort(int port);
    int port();

    void setAuthToken(const QString &token);
    QString authToken();

    //AUTHORIZATION
    void addUser();
    void deleteUser();

    //GET ALL PANEL INFORMATION
    void getControllerInfo();

    //STATES
    void getPower();
    void getHue();
    void getBrightness();
    void getSaturation();
    void getColorTemperature();
    void getColorMode();

    void registerForEvents();
    QUuid setPower(bool power);
    QUuid setColor(QColor color);
    QUuid setHue(int hue);
    QUuid setBrightness(int percentage);
    QUuid setSaturation(int percentage);
    QUuid setMired(int mired);
    QUuid setKelvin(int kelvin);

    //EFFECTS
    void getEffects();
    void getSelectedEffect();
    QUuid setEffect(const QString &effect);

    QUuid identify();

private:
    NetworkAccessManager *m_networkManager = nullptr;
    QString m_authToken;
    QHostAddress m_address;
    int m_port;

signals:
    void connectionChanged(bool connected);
    void authenticationStatusChanged(bool authenticated);
    void requestExecuted(QUuid requestId, bool success);

    void controllerInfoReceived(const ControllerInfo &controllerInfo);
    void authTokenRecieved(const QString &token);
    void powerReceived(bool power);
    void brightnessReceived(int percentage);
    void colorModeReceived(const QString &colorMode);
    void hueReceived(int hue);
    void saturationReceived(int percentage);
    void effectListReceived(const QStringList &effects);
    void colorTemperatureReceived(int kelvin);
    void selectedEffectReceived(const QString &effect);

    //Only supported by Canvas
    void touchEventReceived(GestureID gesture);
};

#endif // NANOLEAF_H
