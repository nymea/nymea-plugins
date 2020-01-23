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

#ifndef HUELIGHT_H
#define HUELIGHT_H

#include <QObject>
#include <QDebug>
#include <QColor>
#include <QPoint>
#include <QHostAddress>
#include <QNetworkRequest>
#include <QJsonDocument>

#include "typeutils.h"
#include "huedevice.h"

class HueLight : public HueDevice
{
    Q_OBJECT
public:

    enum ColorMode {
        ColorModeHS,
        ColorModeXY,
        ColorModeCT
    };

    explicit HueLight(QObject *parent = 0);

    bool power() const;
    void setPower(const bool &power);

    quint8 brightness() const;
    void setBrigtness(const quint8 brightness);

    quint16 hue() const;
    void setHue(const quint16 hue);

    quint8 sat() const;
    void setSat(const quint8 sat);

    QColor color() const;

    QPointF xy() const;
    void setXy(const QPointF &xy);

    quint16 ct() const;
    void setCt(const quint16 &ct);

    QString alert() const;
    void setAlert(const QString &alert);

    QString effect() const;
    void setEffect(const QString &effect);

    ColorMode colorMode() const;
    void setColorMode(const ColorMode &colorMode);

    // update states
    void updateStates(const QVariantMap &statesMap);
    void processActionResponse(const QVariantList &responseList);

    // create action requests
    QPair<QNetworkRequest, QByteArray> createSetPowerRequest(const bool &power);
    QPair<QNetworkRequest, QByteArray> createSetColorRequest(const QColor &color);
    QPair<QNetworkRequest, QByteArray> createSetBrightnessRequest(const int &brightness);
    QPair<QNetworkRequest, QByteArray> createSetEffectRequest(const QString &effect);
    QPair<QNetworkRequest, QByteArray> createSetTemperatureRequest(const int &colorTemp);
    QPair<QNetworkRequest, QByteArray> createFlashRequest(const QString &alert);

private:
    bool m_power;

    quint8 m_brightness;
    quint16 m_hue;
    quint8 m_sat;
    QPointF m_xy;
    quint16 m_ct;
    QString m_alert;
    QString m_effect;
    ColorMode m_colorMode;

signals:
    void stateChanged();

};

#endif // HUELIGHT_H
