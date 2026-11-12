// SPDX-License-Identifier: GPL-3.0-or-later
#include <QtTest>
#include <QDBusConnection>
#include <QDBusMessage>
#include "rfid/rfidproviderclient.h"
#include "rfid/everestrfidutils.h"

class ProviderFixture : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.nymea.RfidProvider1")
    Q_PROPERTY(QString Backend READ backend)
    Q_PROPERTY(bool Ready READ ready)
    Q_PROPERTY(bool SupportsPresence READ supportsPresence)
    Q_PROPERTY(QString LastError READ lastError)
public:
    bool isReady = false;
    QString backend() const { return "fake"; }
    bool ready() const { return isReady; }
    bool supportsPresence() const { return true; }
    QString lastError() const { return {}; }
signals:
    void tagArrived();
    void tagDetected(const QString &code, const QVariantMap &metadata);
    void tagRemoved();
};

class TestRfidProviderClient : public QObject
{
    Q_OBJECT
    QDBusConnection bus = QDBusConnection::sessionBus();
private slots:
    void cleanup()
    {
        bus.unregisterService("io.nymea.RfidProvider");
        bus.unregisterObject("/io/nymea/RfidProvider");
    }

    void absentProvider()
    {
        RfidProviderClient client(nullptr, bus);
        int callbacks = 0;
        bool result = true;
        client.probe([&](bool found) { ++callbacks; result = found; });
        QTRY_COMPARE_WITH_TIMEOUT(callbacks, 1, 2500);
        QVERIFY(!result);
        QVERIFY(!client.available());
        QVERIFY(!client.ready());
    }

    void incompatibleProvider()
    {
        QObject incompatible;
        QVERIFY(bus.registerObject("/io/nymea/RfidProvider", &incompatible, QDBusConnection::ExportAllContents));
        QVERIFY(bus.registerService("io.nymea.RfidProvider"));
        RfidProviderClient client(nullptr, bus);
        int callbacks = 0;
        bool result = true;
        client.probe([&](bool found) { ++callbacks; result = found; });
        QTRY_COMPARE_WITH_TIMEOUT(callbacks, 1, 2500);
        QVERIFY(!result);
        QVERIFY(!client.available());
    }

    void presenceReadinessAndRestart()
    {
        ProviderFixture provider;
        QVERIFY(bus.registerObject("/io/nymea/RfidProvider", &provider, QDBusConnection::ExportAllContents));
        QVERIFY(bus.registerService("io.nymea.RfidProvider"));
        RfidProviderClient client(nullptr, bus);
        int callbacks = 0;
        bool result = false;
        client.probe([&](bool found) { ++callbacks; result = found; });
        QTRY_COMPARE_WITH_TIMEOUT(callbacks, 1, 2500);
        QVERIFY(result);
        QVERIFY(client.available());
        QVERIFY(!client.ready());
        provider.isReady = true;
        auto update = QDBusMessage::createSignal("/io/nymea/RfidProvider", "org.freedesktop.DBus.Properties", "PropertiesChanged");
        update << QString("io.nymea.RfidProvider1") << QVariantMap{{"Ready", true}} << QStringList{};
        QVERIFY(bus.send(update));
        QTRY_VERIFY(client.ready());
        QSignalSpy detections(&client, &RfidProviderClient::tagDetected);
        const QVariantMap metadata{{"technology", "ISO14443A"}};
        emit provider.tagDetected("0001ABFF", metadata);
        QTRY_COMPARE(detections.count(), 1);
        QCOMPARE(detections.first().at(0).toString(), QString("0001ABFF"));
        QCOMPARE(detections.first().at(1).toMap(), metadata);
        QVERIFY(bus.unregisterService("io.nymea.RfidProvider"));
        QTRY_VERIFY(!client.available());
        QVERIFY(!client.ready());
        QVERIFY(bus.registerService("io.nymea.RfidProvider"));
        QTRY_VERIFY_WITH_TIMEOUT(client.available(), 2500);
        QTRY_VERIFY(client.ready());
        // A reconnect must neither replay a detection nor multiply subscriptions.
        QCOMPARE(detections.count(), 1);
        emit provider.tagDetected("0001ABFF", metadata);
        QTRY_COMPARE(detections.count(), 2);
        QTest::qWait(50);
        QCOMPARE(detections.count(), 2);
    }

    void eligibility_data()
    {
        QTest::addColumn<QString>("url");
        QTest::addColumn<int>("evses");
        QTest::addColumn<int>("connectors");
        QTest::addColumn<bool>("ac");
        QTest::addColumn<bool>("dc");
        QTest::addColumn<bool>("eligible");
        QTest::newRow("localhost-ac") << "ws://localhost:8849" << 1 << 1 << true << false << true;
        QTest::newRow("ipv4-ac") << "ws://127.0.0.1:8849" << 1 << 1 << true << false << true;
        QTest::newRow("ipv6-ac") << "ws://[::1]:8849" << 1 << 1 << true << false << true;
        QTest::newRow("remote-ac") << "ws://192.168.1.2:8849" << 1 << 1 << true << false << false;
        QTest::newRow("deceptive-host") << "ws://localhost.example:8849" << 1 << 1 << true << false << false;
        QTest::newRow("multi-evse") << "ws://localhost:8849" << 2 << 1 << true << false << false;
        QTest::newRow("multi-connector") << "ws://localhost:8849" << 1 << 2 << true << false << false;
        QTest::newRow("no-connector") << "ws://localhost:8849" << 1 << 0 << true << false << false;
        QTest::newRow("no-evse") << "ws://localhost:8849" << 0 << 1 << true << false << false;
        QTest::newRow("dc") << "ws://localhost:8849" << 1 << 1 << false << true << false;
        QTest::newRow("mixed-ac-dc") << "ws://localhost:8849" << 1 << 1 << true << true << false;
        QTest::newRow("unknown") << "ws://localhost:8849" << 1 << 1 << false << false << false;
    }

    void eligibility()
    {
        QFETCH(QString, url);
        QFETCH(int, evses);
        QFETCH(int, connectors);
        QFETCH(bool, ac);
        QFETCH(bool, dc);
        QFETCH(bool, eligible);
        QList<EverestJsonRpcClient::EVSEInfo> topology;
        for (int i = 0; i < evses; ++i) {
            EverestJsonRpcClient::EVSEInfo evse;
            evse.index = i;
            for (int j = 0; j < connectors; ++j) {
                EverestJsonRpcClient::ConnectorInfo connector;
                connector.connectorId = j;
                evse.availableConnectors.append(connector);
            }
            if (ac) evse.supportedEnergyTransferModes << EverestJsonRpcClient::EnergyTransferModeAC_three_phase_core;
            if (dc) evse.supportedEnergyTransferModes << EverestJsonRpcClient::EnergyTransferModeDC_core;
            topology << evse;
        }
        QCOMPARE(everestRfidEligible(QUrl(url), topology), eligible);
    }
};
QTEST_GUILESS_MAIN(TestRfidProviderClient)
#include "tst_rfidproviderclient.moc"
