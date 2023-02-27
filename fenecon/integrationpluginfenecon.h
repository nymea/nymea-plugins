/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2022 - 2023, Consolinno Energy GmbH
* Contact: info@consolinno.de
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
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INTEGRATIONPLUGINFENECON_H
#define INTEGRATIONPLUGINFENECON_H

#include "femsconnection.h"
#include "constFemsPaths.h"
#include "integrations/integrationplugin.h"
#include <QHash>
#include <QNetworkReply>
#include <QTimer>
#include <QUuid>

class PluginTimer;

class IntegrationPluginFenecon : public IntegrationPlugin {
  Q_OBJECT

  Q_PLUGIN_METADATA(IID "io.nymea.IntegrationPlugin" FILE
                        "integrationpluginfenecon.json")
  Q_INTERFACES(IntegrationPlugin)

public:


  explicit IntegrationPluginFenecon(QObject *parent = nullptr);

  void init() override;
  void startPairing(ThingPairingInfo *info) override;
  void confirmPairing(ThingPairingInfo *info, const QString &username,
                      const QString &secret) override;
  void setupThing(ThingSetupInfo *info) override;
  void postSetupThing(Thing *thing) override;
  void executeAction(ThingActionInfo *info) override;

  void thingRemoved(Thing *thing) override;

private:
  enum ValueType {
    QSTRING = 0,
    DOUBLE = 1,
    MY_BOOLEAN = 2,
    MY_INT = 3,

  };

  PluginTimer *m_connectionRefreshTimer = nullptr;
  Thing *GetThingByParentAndClassId(Thing *parentThing,
                                    ThingClassId identifier);
  QHash<FemsConnection *, Thing *> m_femsConnections;
  void refreshConnection(FemsConnection *connection);
  void updateSumState(FemsConnection *connection);
  void updateMeters(FemsConnection *connection);
  void updateStorages(FemsConnection *connection);
  bool connectionError(FemsNetworkReply *reply);
  void changeMeterString();
  void checkBatteryState(Thing *parentThing);
  void calculateStateOfHealth(Thing *parentThing);
  int ownId = 0;

  bool jsonError(QByteArray data);

  QString getValueOfRequestedData(QJsonDocument *json);

  void addValueToThing(Thing *parentThing, ThingClassId identifier,
                       StateTypeId stateName, const QVariant *value,
                       ValueType valueType, int scale);

  void addValueToThing(Thing *childThing, StateTypeId stateName,
                       const QVariant *value, ValueType valueType,
                       int scale = 0);

  QString batteryState;

  QString meter;

  bool batteryCreated = false;
  bool meterCreated = false;
  bool statusCreated = false;
};

#endif // INTEGRATIONPLUGINFENECON_H
