// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QTcpServer>
#include <QJsonDocument>
#include <QWebSocket>
#include <QWebSocketServer>

// Construct standalone things without bringing in the server's ThingManager.
#include <types/thingclass.h>
#include <types/state.h>
#define private public
#include <integrations/thing.h>
#undef private

#include "plugininfo.h"
#include "loggingcategories.h"
#include "jsonrpc/everestconnection.h"
#include "jsonrpc/everestjsonrpcclient.h"
#include "jsonrpc/everestevse.h"

NYMEA_LOGGING_CATEGORY(dcEverestTraffic, "EverestTraffic")

class FakeMonitor : public NetworkDeviceMonitor
{
    Q_OBJECT
public:
    explicit FakeMonitor(QObject *parent = nullptr) : NetworkDeviceMonitor(parent)
    {
        m_info.setAddress(QHostAddress::LocalHost);
    }

    MacAddress macAddress() const override { return MacAddress(); }
    QString hostName() const override { return QString(); }
    QHostAddress address() const override { return QHostAddress::LocalHost; }
    NetworkDeviceInfo::MonitorMode monitorMode() const override { return NetworkDeviceInfo::MonitorModeIp; }
    NetworkDeviceInfo networkDeviceInfo() const override { return m_info; }
    bool reachable() const override { return true; }
    QDateTime lastSeen() const override { return QDateTime::currentDateTime(); }
    uint pingRetries() const override { return 0; }
    void setPingRetries(uint) override { }

private:
    NetworkDeviceInfo m_info;
};

// Minimal RpcApi answering the init sequence of EverestJsonRpcClient
class FakeRpcApi : public QObject
{
    Q_OBJECT
public:
    QWebSocketServer server{"fake-everest", QWebSocketServer::NonSecureMode};
    int connectionCount = 0;
    enum HmiResponse { Success, InvalidIndication, NoDataAvailable, Unsupported, Disconnect };
    HmiResponse hmiResponse = Success;
    QList<QVariantMap> hmiRequests;
    QString evseState = "Unplugged";

    bool listen(quint16 port)
    {
        connect(&server, &QWebSocketServer::newConnection, this, [this](){
            QWebSocket *socket = server.nextPendingConnection();
            connectionCount++;
            connect(socket, &QWebSocket::disconnected, socket, &QWebSocket::deleteLater);
            connect(socket, &QWebSocket::textMessageReceived, socket, [this, socket](const QString &message){
                QVariantMap request = QJsonDocument::fromJson(message.toUtf8()).toVariant().toMap();
                QVariantMap result;
                const QString method = request.value("method").toString();
                if (method == "API.Hello") {
                    result = {{"api_version", "1.0"}, {"everest_version", "test"}, {"authentication_required", false}, {"charger_info", QVariantMap{
                                  {"vendor", "PCE"}, {"model", "EV11.4 Wallbox R37 11kW"},
                                  {"serial", "1234567890"}, {"firmware_version", "0.10.9"}}}};
                } else if (method == "ChargePoint.GetEVSEInfos") {
                    result = {{"error", "NoError"}, {"infos", QVariantList()}};
                } else if (method == "EVSE.GetStatus") {
                    result = {{"error", "NoError"}, {"status", QVariantMap{{"state", evseState}}}};
                } else if (method == "ChargePoint.SetHmiIndication") {
                    hmiRequests.append(request);
                    if (hmiResponse == Disconnect) {
                        socket->close();
                        return;
                    }
                    if (hmiResponse == Unsupported) {
                        const QVariantMap response = {{"jsonrpc", "2.0"}, {"id", request.value("id")},
                            {"error", QVariantMap{{"code", -32601}, {"message", "Method not found"}}}};
                        socket->sendTextMessage(QString::fromUtf8(QJsonDocument::fromVariant(response).toJson(QJsonDocument::Compact)));
                        return;
                    }
                    result = {{"error", hmiResponse == InvalidIndication ? "ErrorInvalidParameter" :
                               hmiResponse == NoDataAvailable ? "ErrorNoDataAvailable" : "NoError"}};
                } else {
                    result = {{"error", "NoError"}};
                }
                QVariantMap response = {{"jsonrpc", "2.0"}, {"id", request.value("id")}, {"result", result}};
                socket->sendTextMessage(QString::fromUtf8(QJsonDocument::fromVariant(response).toJson(QJsonDocument::Compact)));
            });
        });
        return server.listen(QHostAddress::LocalHost, port);
    }
};

class TestEverestConnection : public QObject
{
    Q_OBJECT

    static quint16 freePort()
    {
        QTcpServer probe;
        probe.listen(QHostAddress::LocalHost, 0);
        return probe.serverPort();
    }

private slots:
    void observedPlugTransitions_data()
    {
        QTest::addColumn<ThingClassId>("thingClassId");
        QTest::addColumn<QString>("initialState");
        QTest::newRow("pce-unplugged") << everestChargerPceThingClassId << QString("Unplugged");
        QTest::newRow("pce-already-plugged") << everestChargerPceThingClassId << QString("Preparing");
        QTest::newRow("ac-rfid-unplugged") << everestChargerAcRfidThingClassId << QString("Unplugged");
        QTest::newRow("ac-rfid-already-plugged") << everestChargerAcRfidThingClassId << QString("Preparing");
    }

    void observedPlugTransitions()
    {
        QFETCH(ThingClassId, thingClassId);
        QFETCH(QString, initialState);
        ThingClass thingClass(pluginId, ChargebyteVendorId, thingClassId);
        StateTypes stateTypes;
        const QStringList names = {"connected", "pluggedIn", "power", "charging", "state", "sessionEnergy",
            "phaseCount", "desiredPhaseCount", "maxChargingCurrent", "currentPower", "currentPowerPhaseA",
            "currentPowerPhaseB", "currentPowerPhaseC", "currentPhaseA", "currentPhaseB", "currentPhaseC",
            "voltagePhaseA", "voltagePhaseB", "voltagePhaseC", "totalEnergyConsumed"};
        for (const QString &name : names) {
            StateType stateType(StateTypeId::createStateTypeId());
            stateType.setName(name);
            if (name == "state") {
                stateType.setType(QMetaType::QString);
                stateType.setDefaultValue(QString());
            } else if (name == "connected" || name == "pluggedIn" || name == "power" || name == "charging") {
                stateType.setType(QMetaType::Bool);
                stateType.setDefaultValue(false);
            } else {
                stateType.setType(QMetaType::Double);
                stateType.setDefaultValue(0.0);
            }
            stateTypes.append(stateType);
        }
        thingClass.setStateTypes(stateTypes);
        const ParamType indexParam(ParamTypeId::createParamTypeId(), "index", QMetaType::Int, 1);
        thingClass.setParamTypes(ParamTypes() << indexParam);
        Thing thing(pluginId, thingClass);
        States states;
        for (const StateType &stateType : stateTypes) {
            State state(stateType.id(), thing.id());
            states.append(state);
        }
        thing.setStates(states);
        for (const StateType &stateType : stateTypes)
            thing.setStateValue(stateType.id(), stateType.defaultValue());
        thing.setParams(ParamList() << Param(indexParam.id(), 1));

        FakeRpcApi api;
        api.evseState = initialState;
        QVERIFY(api.listen(0));
        const QUrl url(QStringLiteral("ws://127.0.0.1:%1").arg(api.server.serverPort()));
        EverestJsonRpcClient client;
        EverestEvse evse(&client, &thing);
        QSignalSpy pluggedSpy(&evse, &EverestEvse::pluggedInChanged);
        client.connectToServer(url);
        QTRY_VERIFY(thing.stateValue("connected").toBool());
        QCOMPARE(pluggedSpy.count(), 0); // Initial status establishes the baseline.

        auto reportStatus = [&client](const QString &state, int index = 1) {
            emit client.evseStatusChanged(index, EverestJsonRpcClient::parseEvseStatus({{"state", state}}));
        };
        reportStatus(initialState);
        reportStatus("Preparing", 99); // Another EVSE cannot change this one.
        QCOMPARE(pluggedSpy.count(), 0);
        reportStatus("Unplugged");
        pluggedSpy.clear();
        reportStatus("Preparing");
        QCOMPARE(pluggedSpy.count(), 1);
        QCOMPARE(pluggedSpy.last().first().toBool(), true);
        QVERIFY(thing.stateValue("pluggedIn").toBool());
        reportStatus("Preparing");
        reportStatus("Charging");
        QCOMPARE(pluggedSpy.count(), 1);
        reportStatus("Unplugged");
        QCOMPARE(pluggedSpy.count(), 2);
        QCOMPARE(pluggedSpy.last().first().toBool(), false);
        QVERIFY(!thing.stateValue("pluggedIn").toBool());

        client.disconnectFromServer();
        QTRY_VERIFY(!thing.stateValue("connected").toBool());
        reportStatus("Preparing"); // Ignore updates while disconnected.
        QCOMPARE(pluggedSpy.count(), 2);
        api.evseState = "Preparing";
        client.connectToServer(url);
        QTRY_VERIFY(thing.stateValue("connected").toBool());
        QCOMPARE(pluggedSpy.count(), 2); // Reconnect also establishes a baseline.
        reportStatus("Preparing");
        QCOMPARE(pluggedSpy.count(), 2);
        reportStatus("Unplugged");
        reportStatus("Preparing");
        QCOMPARE(pluggedSpy.count(), 4);
        QCOMPARE(pluggedSpy.last().first().toBool(), true);
    }

    void hmiIndicationWireContract()
    {
        FakeRpcApi api;
        QVERIFY(api.listen(0));
        EverestJsonRpcClient client;
        client.connectToServer(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(api.server.serverPort())));
        QTRY_VERIFY(client.available());

        const QStringList indications = {"TagEnrollmentActive", "TagEnrollmentInactive", "AuthRequired",
                                         "AuthAccepted", "AuthRejected", "AuthRejected"};
        QSet<int> requestIds;
        for (const QString &indication : indications) {
            EverestJsonRpcReply *reply = client.chargePointSetHmiIndication(indication);
            QSignalSpy finishedSpy(reply, &EverestJsonRpcReply::finished);
            QVERIFY(!reply->retry());
            QTRY_COMPARE(finishedSpy.count(), 1);
            QCOMPARE(reply->error(), EverestJsonRpcReply::ErrorNoError);
            QCOMPARE(reply->response().value("result").toMap().value("error").toString(), QString("NoError"));
            QCOMPARE(api.hmiRequests.last().value("method").toString(), QString("ChargePoint.SetHmiIndication"));
            QCOMPARE(api.hmiRequests.last().value("params").toMap(), QVariantMap({{"indication", indication}}));
            QCOMPARE(api.hmiRequests.last().value("jsonrpc").toString(), QString("2.0"));
            requestIds.insert(api.hmiRequests.last().value("id").toInt());
            delete reply;
        }
        QCOMPARE(api.hmiRequests.count(), indications.count());
        QCOMPARE(requestIds.count(), indications.count());
    }

    void hmiIndicationErrors_data()
    {
        QTest::addColumn<int>("response");
        QTest::addColumn<int>("expectedError");
        QTest::addColumn<QString>("resultError");
        QTest::newRow("invalid-indication") << int(FakeRpcApi::InvalidIndication)
            << int(EverestJsonRpcReply::ErrorNoError) << QString("ErrorInvalidParameter");
        QTest::newRow("no-data-no-retry") << int(FakeRpcApi::NoDataAvailable)
            << int(EverestJsonRpcReply::ErrorNoError) << QString("ErrorNoDataAvailable");
        QTest::newRow("unsupported-method") << int(FakeRpcApi::Unsupported)
            << int(EverestJsonRpcReply::ErrorJsonRpcError) << QString();
        QTest::newRow("connection-lost") << int(FakeRpcApi::Disconnect)
            << int(EverestJsonRpcReply::ErrorConnectionError) << QString();
    }

    void hmiIndicationErrors()
    {
        QFETCH(int, response);
        QFETCH(int, expectedError);
        QFETCH(QString, resultError);
        FakeRpcApi api;
        api.hmiResponse = static_cast<FakeRpcApi::HmiResponse>(response);
        QVERIFY(api.listen(0));
        EverestJsonRpcClient client;
        client.connectToServer(QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(api.server.serverPort())));
        QTRY_VERIFY(client.available());

        EverestJsonRpcReply *reply = client.chargePointSetHmiIndication("AuthRejected");
        QSignalSpy finishedSpy(reply, &EverestJsonRpcReply::finished);
        QVERIFY(!reply->retry());
        QTRY_COMPARE(finishedSpy.count(), 1);
        QCOMPARE(int(reply->error()), expectedError);
        QCOMPARE(reply->response().value("result").toMap().value("error").toString(), resultError);
        QCOMPARE(reply->retryCount(), 0);
        QCOMPARE(api.hmiRequests.count(), 1);
        if (response != FakeRpcApi::Disconnect)
            QVERIFY(client.available());
        delete reply;
    }

    void reconnectAfterConnectionRefused()
    {
        const quint16 port = freePort();
        QVERIFY(port != 0);

        FakeMonitor monitor;
        EverestConnection connection(port);
        connection.setMonitor(&monitor);
        QSignalSpy errorSpy(connection.client(), &EverestJsonRpcClient::connectionErrorOccurred);
        QSignalSpy availableSpy(&connection, &EverestConnection::availableChanged);

        // Server not up yet, like EVerest still booting
        connection.start();
        QTRY_VERIFY_WITH_TIMEOUT(errorSpy.count() >= 1, 2000);
        QVERIFY(!connection.available());

        FakeRpcApi api;
        QVERIFY(api.listen(port));

        QTRY_VERIFY_WITH_TIMEOUT(connection.available(), 8000);
        QCOMPARE(availableSpy.count(), 1);
        QCOMPARE(availableSpy.first().first().toBool(), true);

        const EverestJsonRpcClient::ChargerInfo chargerInfo = connection.client()->chargerInfo();
        QCOMPARE(chargerInfo.vendor, QString("PCE"));
        QCOMPARE(chargerInfo.model, QString("EV11.4 Wallbox R37 11kW"));
        QCOMPARE(chargerInfo.serialNumber, QString("1234567890"));
        QCOMPARE(chargerInfo.firmwareVersion, QString("0.10.9"));
    }

    void noReconnectWhenStopped()
    {
        const quint16 port = freePort();
        QVERIFY(port != 0);

        FakeMonitor monitor;
        EverestConnection connection(port);
        connection.setMonitor(&monitor);
        QSignalSpy errorSpy(connection.client(), &EverestJsonRpcClient::connectionErrorOccurred);

        connection.start();
        QTRY_VERIFY_WITH_TIMEOUT(errorSpy.count() >= 1, 2000);
        connection.stop();

        FakeRpcApi api;
        QVERIFY(api.listen(port));

        QTest::qWait(6000);
        QVERIFY(!connection.available());
        QCOMPARE(api.connectionCount, 0);
    }
};

QTEST_GUILESS_MAIN(TestEverestConnection)
#include "tst_everestconnection.moc"
