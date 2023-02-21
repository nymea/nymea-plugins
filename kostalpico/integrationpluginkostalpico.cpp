/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2020 Consolinno Energy GmbH <f.stoecker@consolinno.de> *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation;                  *
 *  version 3 of the License.                                              *
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

#include "integrationpluginkostalpico.h"
#include "plugininfo.h"
#include "plugintimer.h"
#include <QNetworkInterface>
#include <QXmlStreamReader>
#include <network/networkdevicediscovery.h>

IntegrationPluginKostal::IntegrationPluginKostal() {}

void IntegrationPluginKostal::discoverThings(ThingDiscoveryInfo *info) {
  if (info->thingClassId() == connectionThingClassId) {
    if (!hardwareManager()->networkDeviceDiscovery()->available()) {
      qCWarning(dcKostalpico())
          << "Failed to discover network devices. The network device discovery "
             "is not available.";
      info->finish(Thing::ThingErrorHardwareNotAvailable,
                   QT_TR_NOOP("The discovery is not available. Please enter "
                              "the IP address manually."));
      return;
    }

    qCDebug(dcKostalpico()) << "Starting network discovery...";
    NetworkDeviceDiscoveryReply *discoveryReply =
        hardwareManager()->networkDeviceDiscovery()->discover();
    connect(discoveryReply, &NetworkDeviceDiscoveryReply::finished,
            discoveryReply, &NetworkDeviceDiscoveryReply::deleteLater);
    connect(
        discoveryReply, &NetworkDeviceDiscoveryReply::finished, info, [=]() {
          qCDebug(dcKostalpico())
              << "Discovery finished. Found"
              << discoveryReply->networkDeviceInfos().count() << "devices";
          foreach (const NetworkDeviceInfo &networkDeviceInfo,
                   discoveryReply->networkDeviceInfos()) {
            qCDebug(dcKostalpico()) << networkDeviceInfo;

            QString title;
            if (networkDeviceInfo.hostName().isEmpty()) {
              title = networkDeviceInfo.address().toString();
            } else {
              title = networkDeviceInfo.hostName() + " (" +
                      networkDeviceInfo.address().toString() + ")";
            }

            QString description;
            if (networkDeviceInfo.macAddressManufacturer().isEmpty()) {
              description = networkDeviceInfo.macAddress();
            } else {
              description = networkDeviceInfo.macAddress() + " (" +
                            networkDeviceInfo.macAddressManufacturer() + ")";
            }

            ThingDescriptor descriptor(info->thingClassId(), title,
                                       description);
            ParamList params;
            params << Param(connectionThingMacAddressParamTypeId,
                            networkDeviceInfo.macAddress());
            descriptor.setParams(params);

            // Check if we already have set up this device
            Things existingThings =
                myThings().filterByParam(connectionThingMacAddressParamTypeId,
                                         networkDeviceInfo.macAddress());
            if (existingThings.count() == 1) {
              qCDebug(dcKostalpico())
                  << "This connection already exists in the system:"
                  << networkDeviceInfo;
              descriptor.setThingId(existingThings.first()->id());
            }

            info->addThingDescriptor(descriptor);
          }
          info->finish(Thing::ThingErrorNoError);
        });
  }
}

void IntegrationPluginKostal::setupThing(ThingSetupInfo *info) {
  // A thing is being set up. Use info->thing() to get details of the thing, do
  // the required setup (e.g. connect to the device) and call info->finish()
  // when done.
  Thing *thing = info->thing();
  qCDebug(dcKostalpico()) << "Setup thing" << info->thing();

  if (thing->thingClassId() == connectionThingClassId) {

    MacAddress mac = MacAddress(
        thing->paramValue(connectionThingMacAddressParamTypeId).toString());
    if (!mac.isValid()) {
      info->finish(Thing::ThingErrorInvalidParameter,
                   QT_TR_NOOP("The given MAC address is not valid."));
      return;
    }
    NetworkDeviceMonitor *monitor =
        hardwareManager()->networkDeviceDiscovery()->registerMonitor(mac);
    connect(info, &ThingSetupInfo::aborted, monitor, [monitor, this]() {
      hardwareManager()->networkDeviceDiscovery()->unregisterMonitor(monitor);
    });

    // Handle reconfigure
    m_connectionThing->deleteLater();
    qCDebug(dcKostalpico()) << "Added DeviceAddress: "
                        << monitor->networkDeviceInfo().address().toString();
    // Create the connection
    KostalPicoConnection *connection =
        new KostalPicoConnection(hardwareManager()->networkManager(),
                                 monitor->networkDeviceInfo().address(), thing);

    // (possible TODO, depends if API changes with versions -> Verify the
    // version ?)
    KostalNetworkReply *reply = connection->getActiveDevices();
    connect(
        reply, &KostalNetworkReply::finished, info,
        [this, reply, info, connection, thing] {
          QByteArray data = reply->networkReply()->readAll();
          if (reply->networkReply()->error() != QNetworkReply::NoError) {
            qCWarning(dcKostalpico())
                << "Network request error:" << reply->networkReply()->error()
                << reply->networkReply()->errorString()
                << reply->networkReply()->url();
            if (reply->networkReply()->error() ==
                QNetworkReply::ContentNotFoundError) {
              info->finish(
                  Thing::ThingErrorHardwareNotAvailable,
                  QT_TR_NOOP(
                      "The device does not reply to our requests. Please "
                      "verify "
                      "that the Fronius Solar API is enabled on the device."));
            } else {
              info->finish(Thing::ThingErrorHardwareNotAvailable,
                           QT_TR_NOOP("The device is not reachable."));
            }
            return;
          }

          // Convert the rawdata to an xml document -> see if no Error

          QXmlStreamReader *xmlDoc = new QXmlStreamReader(data);
          if (xmlDoc->hasError()) {
            qCWarning(dcKostalpico()) << "Failed to parse XML data" << data;
            info->finish(
                Thing::ThingErrorHardwareFailure,
                QT_TR_NOOP("The data received from the device could not "
                           "be processed because the format is unknown."));
            return;
          }
          info->finish(Thing::ThingErrorNoError);
          this->m_kostalConnection = connection;
          this->m_connectionThing = thing;

          // Update the already known states
          thing->setStateValue("connected", true);
        });

    connect(connection, &KostalPicoConnection::availableChanged, this,
            [this, thing](bool available) {
              qCDebug(dcKostalpico()) << thing << "Available changed" << available;
              thing->setStateValue("connected", available);
              // Update all child things, they will be set to available once
              // the connection starts working again
              foreach (Thing *thing, myThings().filterByThingClassId(
                                         kostalpicoThingClassId)) {
                thing->setStateValue("connected", available);
              }
            });

  } else if ((thing->thingClassId() == kostalpicoThingClassId)) {
    qCDebug(dcKostalpico()) << "Children Thing detected";
    // Verify the parent connection

    KostalPicoConnection *connection = this->m_kostalConnection;
    if (!connection) {
      qCWarning(dcKostalpico()) << "Could not find the connection for" << thing;
      info->finish(Thing::ThingErrorHardwareNotAvailable);
      return;
    }
    info->finish(Thing::ThingErrorNoError);

  } else {
    Q_ASSERT_X(false, "setupThing",
               QString("Unhandled thingClassId: %1")
                   .arg(thing->thingClassId().toString())
                   .toUtf8());
  }
}

void IntegrationPluginKostal::postSetupThing(Thing *thing) {
  qCDebug(dcKostalpico()) << "Post setup" << thing->name();

  if (thing->thingClassId() == connectionThingClassId) {

    // Create a refresh timer for monitoring the active devices
    if (!m_connectionRefreshTimer) {
      m_connectionRefreshTimer =
          hardwareManager()->pluginTimerManager()->registerTimer(2);
      connect(m_connectionRefreshTimer, &PluginTimer::timeout, this,
              [this]() { this->refreshConnection(); });
      m_connectionRefreshTimer->start();
    }

    // Refresh now
    KostalPicoConnection *connection = m_kostalConnection;

    if (connection) {
      qCDebug(dcKostalpico()) << "Refreshing Connection";
      this->refreshConnection();
    }
  } else if (thing->thingClassId() == kostalpicoThingClassId) {
    thing->setStateValue("connected", true);
  }
}

void IntegrationPluginKostal::executeAction(ThingActionInfo *info) {
  Q_UNUSED(info)
}

void IntegrationPluginKostal::thingRemoved(Thing *thing) {
  qCDebug(dcKostalpico()) << "Removing Thing";
  if (thing->thingClassId() == connectionThingClassId) {
    KostalPicoConnection *connection = m_kostalConnection;
    connection->deleteLater();
    this->m_connectionThing->deleteLater();
  }

  if (myThings().filterByThingClassId(connectionThingClassId).isEmpty()) {
    hardwareManager()->pluginTimerManager()->unregisterTimer(
        m_connectionRefreshTimer);
    m_connectionRefreshTimer = nullptr;
  }
}
void IntegrationPluginKostal::refreshConnection() {
  if (this->m_kostalConnection->busy()) {
    qCWarning(dcKostalpico()) << "Connection busy. Skipping refresh cycle for host"
                          << this->m_kostalConnection->address().toString();
    return;
  }
  qCDebug(dcKostalpico()) << "Creating reply and getting all Active Devices";
  // Note: this call will be used to monitor the available state of the
  // connection internally
  KostalNetworkReply *reply = this->m_kostalConnection->getActiveDevices();
  connect(reply, &KostalNetworkReply::finished, this, [=]() {
    if (reply->networkReply()->error() != QNetworkReply::NoError) {
      // Note: the connection warns about any errors if available changed
      return;
    }

    Thing *connectionThing = this->m_connectionThing;
    if (!connectionThing)
      return;

    QByteArray data = reply->networkReply()->readAll();

    QXmlStreamReader *xmlDoc = new QXmlStreamReader(data);
    if (xmlDoc->hasError()) {
      qCWarning(dcKostalpico())
          << "Failed to parse XML data" << data << ":" << xmlDoc->error();
      return;
    }
    bool childEmpty =
        myThings().filterByThingClassId(kostalpicoThingClassId).isEmpty();
    qCDebug(dcKostalpico()) << "Looking if ChildrenThing is empty";
    qCDebug(dcKostalpico()) << childEmpty;
    if (childEmpty) {
      qCDebug(dcKostalpico()) << "Creating Inverter";
      QString thingDescription = connectionThing->name();
      ThingDescriptor descriptor(kostalpicoThingClassId, "Kostal Pico MP Plus",
                                 thingDescription, connectionThing->id());
      ParamList params;
      params.append(Param(kostalpicoThingIdParamTypeId, "Smartmeter"));
      descriptor.setParams(params);
      emit autoThingsAppeared(ThingDescriptors() << descriptor);
    }
    // only update if child appeared
    else {
      // qCDebug(dcKostalpico()) << "Updating Datapoints";
      // All devices
      if (this->m_toggle) {
        updateCurrentPower(this->m_kostalConnection);
        this->m_toggle = false;
      } else {
        updateTotalEnergyProduced(this->m_kostalConnection);
        this->m_toggle = true;
      }
    }
  });
}

// Consumption
// Look for Value in <Measurement Value='XYZ' Unit='W'
// Type='GridConsumedPower'/>
void IntegrationPluginKostal::updateCurrentPower(
    KostalPicoConnection *connection) {
  KostalNetworkReply *currentPower = connection->getMeasurement();
  connect(
      currentPower, &KostalNetworkReply::finished, this,
      [this, currentPower]() {
        if (currentPower->networkReply()->error() != QNetworkReply::NoError) {
          return;
        }
        QByteArray data = currentPower->networkReply()->readAll();

        QXmlStreamReader *xmlDoc = new QXmlStreamReader(data);
        if (xmlDoc->hasError()) {
          qCWarning(dcKostalpico())
              << "Failed to parse XML data" << data << ":" << xmlDoc->error();
          return;
        }
        qCDebug(dcKostalpico()) << "Reading Measurements XML";
        // TODO XML LOGIC
        QString currentPower = "";
        bool entryFound = false;
        while (xmlDoc->readNextStartElement() && !entryFound) {
          qCDebug(dcKostalpico())
              << "Reading Next Start Element: " << xmlDoc->name();
          if (xmlDoc->name() == "Measurements") {
            while (!xmlDoc->atEnd() && xmlDoc->readNext() && !entryFound) {
              if (xmlDoc->name() == "Measurement" &&
                  xmlDoc->attributes().hasAttribute("Type")) {
                qCDebug(dcKostalpico()) << "Found potential Measurement";
                if (xmlDoc->attributes().value("Type") == "AC_Power") {
                  qCDebug(dcKostalpico()) << "Found correct XML Entry for AC Power";

                  if (xmlDoc->attributes().hasAttribute("Value")) {
                    currentPower =
                        xmlDoc->attributes().value("Value").toString();
                    qCDebug(dcKostalpico())
                        << "Found Value for Consumption" << currentPower;
                    entryFound = true;
                    break;
                  } else {
                    qCDebug(dcKostalpico())
                        << "Not Value Entry for XML Entry Consumed Power";
                  }
                } else {
                  qCDebug(dcKostalpico())
                      << "Was not the correct Measurement" << xmlDoc->name();
                  qCDebug(dcKostalpico()) << "Attributes were: ";
                  int count = xmlDoc->attributes().count();
                  for (int i = 0; i < count; i++) {
                    qCDebug(dcKostalpico())
                        << "Attribute at " << i << " is: "
                        << xmlDoc->attributes().at(i).name().toString()
                        << " with a value of: "
                        << xmlDoc->attributes().at(i).value().toString();
                    ;
                  }
                }
              }
            }
          }
        }

        if (entryFound) {
          qCDebug(dcKostalpico()) << "Value for Consumption found " << currentPower;
          Thing *kostalPicoInverter =
              myThings().filterByThingClassId(kostalpicoThingClassId).first();
          if (kostalPicoInverter != nullptr) {
            qCDebug(dcKostalpico())
                << "Found KostalPicoInverter and add Consumption";
            double currentPowerDouble = currentPower.toDouble();
            currentPowerDouble = currentPowerDouble * -1.0;
            kostalPicoInverter->setStateValue(kostalpicoCurrentPowerStateTypeId,
                                              currentPowerDouble);
          }
        }
      });
}
// Production
// Look for
//<Yields>
//<Yield Type='Produced' Slot='Total' Unit='Wh'>
//  <YieldValue Value='2547230' TimeStamp='2022-06-22T17:30:00'/>
//</Yield>
void IntegrationPluginKostal::updateTotalEnergyProduced(
    KostalPicoConnection *connection) {
  KostalNetworkReply *totalEnergyConsumed = connection->getYields();
  connect(
      totalEnergyConsumed, &KostalNetworkReply::finished, this,
      [this, totalEnergyConsumed]() {
        if (totalEnergyConsumed->networkReply()->error() !=
            QNetworkReply::NoError) {
          return;
        }
        QByteArray data = totalEnergyConsumed->networkReply()->readAll();

        QXmlStreamReader *xmlDoc = new QXmlStreamReader(data);
        if (xmlDoc->hasError()) {
          qCWarning(dcKostalpico())
              << "Failed to parse XML data" << data << ":" << xmlDoc->error();
          return;
        }

        bool entryFound = false;
        QString totalProducedEnergy = "";
        qCDebug(dcKostalpico()) << "Starting with Yields";
        while (xmlDoc->readNextStartElement() && !entryFound) {
          qCDebug(dcKostalpico())
              << "Reading Next Start Element: " << xmlDoc->name();
          if (xmlDoc->name() == "Yield" &&
              xmlDoc->attributes().hasAttribute("Type") &&
              xmlDoc->attributes().hasAttribute("Slot")) {
            qCDebug(dcKostalpico()) << "Found potential Yield";
            if (xmlDoc->attributes().value("Type") == "Produced" &&
                xmlDoc->attributes().value("Slot") == "Total") {
              qCDebug(dcKostalpico()) << "Found correct XML Entry for Produced";
              if (xmlDoc->readNextStartElement()) {
                if (xmlDoc->name() == "YieldValue" &&
                    xmlDoc->attributes().hasAttribute("Value")) {
                  totalProducedEnergy =
                      xmlDoc->attributes().value("Value").toString();
                  qCDebug(dcKostalpico())
                      << "Found Value for Production" << totalProducedEnergy;
                  entryFound = true;
                  break;
                }
              }
            }
          }
        }
        if (entryFound) {
          qCDebug(dcKostalpico())
              << "Entry found writing Value : " << totalProducedEnergy;
          Thing *kostalPicoInverter =
              myThings().filterByThingClassId(kostalpicoThingClassId).first();
          if (kostalPicoInverter != nullptr)
            qCDebug(dcKostalpico()) << "Thing KostalPicoInvertert found.";
          double totalWh = totalProducedEnergy.toDouble();
          double totalkWh = totalWh / 1000;
          qCDebug(dcKostalpico()) << totalkWh;
          kostalPicoInverter->setStateValue(
              kostalpicoTotalEnergyProducedStateTypeId, totalkWh);
        }
      });
}
