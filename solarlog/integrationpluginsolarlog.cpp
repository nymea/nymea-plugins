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

#include "integrationpluginsolarlog.h".h"
#include "plugininfo.h"
#include "network/networkaccessmanager.h"


IntegrationPluginSolarLog::IntegrationPluginSolarLog()
{

}

void IntegrationPluginSolarLog::setupThing(ThingSetupInfo *info)
{
    if (!m_refreshTimer) {
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(2);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &IntegrationPluginSolarLog::onRefreshTimer);
    }
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginSolarLog::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)

    if (myThings().isEmpty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }
}

void IntegrationPluginSolarLog::onRefreshTimer()
{
    foreach (Thing *, myThings().filterByThingClassId()) {

    }
}

QUuid IntegrationPluginSolarLog::getData(const QHostAddress &address)
{
    QUrl url;
    url.setHost(address);
    url.setPath("/getjp");
    url.setScheme("http");

    hardwareManager()->networkManager()->post(QNetworkRequest(url), QByteArray("{\"801\":{\"170\":null}}"));
}


