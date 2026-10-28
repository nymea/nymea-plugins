// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QTcpServer>
#include <QJsonDocument>
#include <QWebSocket>
#include <QWebSocketServer>

#include "plugininfo.h"
#include "loggingcategories.h"
#include "jsonrpc/everestconnection.h"
#include "jsonrpc/everestjsonrpcclient.h"

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

    bool listen(quint16 port)
    {
        connect(&server, &QWebSocketServer::newConnection, this, [this](){
            QWebSocket *socket = server.nextPendingConnection();
            connectionCount++;
            connect(socket, &QWebSocket::disconnected, socket, &QWebSocket::deleteLater);
            connect(socket, &QWebSocket::textMessageReceived, socket, [socket](const QString &message){
                QVariantMap request = QJsonDocument::fromJson(message.toUtf8()).toVariant().toMap();
                QVariantMap result;
                const QString method = request.value("method").toString();
                if (method == "API.Hello") {
                    result = {{"api_version", "1.0"}, {"everest_version", "test"}, {"authentication_required", false}, {"charger_info", QVariantMap{
                                  {"vendor", "PCE"}, {"model", "EV11.4 Wallbox R37 11kW"},
                                  {"serial", "1234567890"}, {"firmware_version", "0.10.9"}}}};
                } else if (method == "ChargePoint.GetEVSEInfos") {
                    result = {{"error", "NoError"}, {"infos", QVariantList()}};
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
