/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2025, nymea GmbH
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

#ifndef AIRQUALITYINDEX_H
#define AIRQUALITYINDEX_H


#include <QObject>
#include <QUuid>
#include <QTime>

#include <network/networkaccessmanager.h>

class AirQualityIndex : public QObject
{
    Q_OBJECT
public:
    struct AirQualityData {
        double humidity;
        double pressure;
        int pm25;
        int pm10;
        double so2;
        double no2;
        double o3;
        double co;
        double temperature;
        double windSpeed;
    };

    struct GeoData {
      double latitude;
      double longitude;
    };

    struct Station {
        int idx;
        int aqi;
        QTime measurementTime;
        QString timezone;
        QString name;
        GeoData location;
        QUrl url;
    };

    explicit AirQualityIndex(NetworkAccessManager *networkAccessManager, const QString &apiKey, QObject *parent = nullptr);

    void setApiKey(const QString &apiKey);
    QUuid searchByName(const QString &name);
    QUuid getDataByIp();
    QUuid getDataByGeolocation(double lat, double lng);

private:
    NetworkAccessManager *m_networkAccessManager;
    QString m_baseUrl = "https://api.waqi.info";
    QString m_apiKey;

    bool parseData(QUuid requestId, const QByteArray &data);

signals:
    void stationsReceived(QUuid requestId, QList<Station> stations);
    void requestExecuted(QUuid requestId, bool success);
    void dataReceived(QUuid requestId, const AirQualityData &data);
};

#endif // AIRQUALITYINDEX_H
