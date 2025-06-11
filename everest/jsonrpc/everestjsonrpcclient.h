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

#ifndef EVERESTJSONRPCCLIENT_H
#define EVERESTJSONRPCCLIENT_H

#include <QObject>

#include <integrations/thing.h>
#include <network/macaddress.h>
#include <network/networkdevicemonitor.h>

#include "everestjsonrpcreply.h"
#include "everestjsonrpcinterface.h"

class EverestJsonRpcClient : public QObject
{
    Q_OBJECT
public:
    // API Enums

    enum ConnectorType {
        ConnectorTypecCCS1,
        ConnectorTypecCCS2,
        ConnectorTypecG105,
        ConnectorTypecTesla,
        ConnectorTypecType1,
        ConnectorTypecType2,
        ConnectorTypes309_1P_16A,
        ConnectorTypes309_1P_32A,
        ConnectorTypes309_3P_16A,
        ConnectorTypes309_3P_32A,
        ConnectorTypesBS1361,
        ConnectorTypesCEE_7_7,
        ConnectorTypesType2,
        ConnectorTypesType3,
        ConnectorTypeOther1PhMax16A,
        ConnectorTypeOther1PhOver16A,
        ConnectorTypeOther3Ph,
        ConnectorTypePan,
        ConnectorTypewInductive,
        ConnectorTypewResonant,
        ConnectorTypeUndetermined
    };
    Q_ENUM(ConnectorType)

    // API Objects

    typedef struct ChargerInfo {
        QString vendor;
        QString model;
        QString serialNumber;
        QString firmwareVersion;
    } ChargerInfo;

    typedef struct ConnectorInfo {
        int connectorId = -1;
        ConnectorType type = ConnectorTypeUndetermined;
        QString description; // optional
    } ConnectorInfo;

    typedef struct EVSEInfo {
        int index = -1;
        QString id;
        QString description; // optional
        bool bidirectionalCharging = false;
        QList<ConnectorInfo> availableConnectors;
    } EVSEInfo;


    explicit EverestJsonRpcClient(QObject *parent = nullptr);

    QUrl serverUrl();

    bool available() const;

    // Once available, following properties are set
    QString apiVersion() const;
    QString everestVersion() const;
    ChargerInfo chargerInfo() const;

    QList<EVSEInfo> evseInfos() const;

    // API calls
    EverestJsonRpcReply *apiHello();
    EverestJsonRpcReply *chargePointGetEVSEInfos();

    EverestJsonRpcReply *evseGetInfo();
    EverestJsonRpcReply *evseGetStatus(int evseIndex);
    EverestJsonRpcReply *evseGetHardwareCapabilities(int evseIndex);

public slots:
    void connectToServer(const QUrl &serverUrl);
    void disconnectFromServer();

signals:
    void connectionErrorOccurred();
    void availableChanged(bool available);

private slots:
    void sendRequest(EverestJsonRpcReply *reply);
    void processDataPacket(const QByteArray &data);

private:
    bool m_available = false;
    int m_commandId = 0;
    EverestJsonRpcInterface *m_interface = nullptr;
    QHash<int, EverestJsonRpcReply *> m_replies;

    // Init infos
    QString m_apiVersion;
    QString m_everestVersion;
    ChargerInfo m_chargerInfo;
    bool m_authenticationRequired = false;
    QList<EVSEInfo> m_evseInfos;

    // API parser methods
    EVSEInfo parseEvseInfo(const QVariantMap &evseInfoMap);
    ConnectorInfo parseConnectorInfo(const QVariantMap &connectorInfoMap);
    ConnectorType parseConnectorType(const QString &connectorTypeString);
};

#endif // EVERESTJSONRPCCLIENT_H
