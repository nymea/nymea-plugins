// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef NANOLEAF_H
#define NANOLEAF_H

#include <QObject>
#include <QTimer>
#include <QUuid>
#include <QHostAddress>
#include <QColor>

#include <network/networkaccessmanager.h>
#include <integrations/thing.h>

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

    enum ColorMode {
        EffectMode,
        HueSaturationMode,
        ColorTemperatureMode
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

    void controllerInfoReceived(const Nanoleaf::ControllerInfo &controllerInfo);
    void authTokenRecieved(const QString &token);
    void powerReceived(bool power);
    void brightnessReceived(int percentage);
    void colorModeReceived(Nanoleaf::ColorMode colorMode);
    void hueReceived(int hue);
    void saturationReceived(int percentage);
    void effectListReceived(const QStringList &effects);
    void colorReceived(QColor color);
    void colorTemperatureReceived(int kelvin);
    void selectedEffectReceived(const QString &effect);

    //Only supported by Canvas
    void touchEventReceived(Nanoleaf::GestureID gesture);
};

#endif // NANOLEAF_H
