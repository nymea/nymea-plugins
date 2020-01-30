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

#ifndef DEVICEPLUGINOPENWEATHERMAP_H
#define DEVICEPLUGINOPENWEATHERMAP_H

#include "plugintimer.h"
#include "devices/deviceplugin.h"
#include "network/networkaccessmanager.h"

#include <QTimer>
#include <QHostAddress>

class DevicePluginOpenweathermap : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.DevicePlugin" FILE "devicepluginopenweathermap.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginOpenweathermap();
    ~DevicePluginOpenweathermap();

    void init() override;
    void discoverDevices(DeviceDiscoveryInfo *info) override;
    void setupDevice(DeviceSetupInfo *info) override;
    void executeAction(DeviceActionInfo *info) override;

    void deviceRemoved(Device *device) override;

private:
    PluginTimer *m_pluginTimer = nullptr;
    QHash<QNetworkReply *, Device *> m_weatherReplies;

    QString m_apiKey;

    void update(Device *device);
    void searchAutodetect(DeviceDiscoveryInfo *info);
    void search(QString searchString, DeviceDiscoveryInfo *info);
    void searchGeoLocation(double lat, double lon, const QString &country, DeviceDiscoveryInfo *info);

    void processAutodetectResponse(QByteArray data);
    void processSearchResponse(QByteArray data);
    void processGeoSearchResponse(QByteArray data);

    void processSearchResults(const QList<QVariantMap> &cityList, DeviceDiscoveryInfo *info);
    void processWeatherData(const QByteArray &data, Device *device);

private slots:
    void networkManagerReplyReady();
    void onPluginTimer();

};

#endif // DEVICEPLUGINOPENWEATHERMAP_H
