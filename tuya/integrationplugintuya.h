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

#ifndef INTEGRATIONPLUGINTUYA_H
#define INTEGRATIONPLUGINTUYA_H

#include <QTimer>

#include "integrations/integrationplugin.h"

class PluginTimer;

class IntegrationPluginTuya: public IntegrationPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE "integrationplugintuya.json")
    Q_INTERFACES(IntegrationPlugin)


public:
    explicit IntegrationPluginTuya(QObject *parent = nullptr);
    ~IntegrationPluginTuya() override;

    void setupThing(ThingSetupInfo *info) override;
    void postSetupThing(Thing *thing) override;
    void thingRemoved(Thing *thing) override;
    void startPairing(ThingPairingInfo *info) override;
    void confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret) override;
    void executeAction(ThingActionInfo *info) override;

signals:
    void tokenRefreshed(Thing *thing, bool success);

private:
    void refreshAccessToken(Thing *thing);
    void updateChildDevices(Thing *thing);
    void queryDevice(Thing *thing);

    void controlTuyaSwitch(const QString &devId, const QString &command, const QVariant &value, ThingActionInfo *info);

    QHash<ThingId, QTimer*> m_tokenExpiryTimers;
    PluginTimer *m_pluginTimerQuery = nullptr;
    PluginTimer *m_pluginTimerDiscovery = nullptr;

    QHash<Thing*, QList<Thing*>> m_pollQueue;
};

#endif // INTEGRATIONPLUGINTUYA_H
