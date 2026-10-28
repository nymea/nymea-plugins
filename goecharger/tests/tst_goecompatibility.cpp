// SPDX-License-Identifier: GPL-3.0-or-later

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright (C) 2013 - 2024, nymea GmbH
* Copyright (C) 2024 - 2025, chargebyte austria GmbH
*
* This file is part of nymea-plugins.
*
* nymea-plugins is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* nymea-plugins is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with nymea-plugins. If not, see <https://www.gnu.org/licenses/>.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include "goediscovery.h"
#include "goestatus.h"

class TestGoeCompatibility : public QObject
{
    Q_OBJECT
private slots:
    void discoveryMerge()
    {
        for (bool zeroConfFirst : {false, true}) {
            GoeDiscoveryData::Results<GoeDiscovery::Result> discovery;
            GoeDiscovery::Result http;
            http.address = QHostAddress("192.0.2.1");
            http.serialNumber = "604415";
            http.firmwareVersion = "60.4";
            http.friendlyName = "HTTP name";
            http.apiAvailableV2 = true;
            GoeDiscovery::Result service;
            service.address = http.address;
            service.discoveryMethod = GoeDiscovery::DiscoveryMethodZeroConf;
            service.friendlyName = "TXT name";
            if (zeroConfFirst) {
                discovery.add(service, GoeDiscoveryData::SourceZeroConf);
                QVERIFY(discovery.results().isEmpty());
            }
            discovery.add(http, GoeDiscoveryData::SourceApiV2);
            if (!zeroConfFirst)
                discovery.add(service, GoeDiscoveryData::SourceZeroConf);
            service.friendlyName.clear(); // A later SSH advertisement must not erase TXT.
            discovery.add(service, GoeDiscoveryData::SourceZeroConf);
            GoeDiscovery::Result v1 = http;
            v1.firmwareVersion = "old";
            v1.apiAvailableV2 = false;
            v1.apiAvailableV1 = true;
            discovery.add(v1, GoeDiscoveryData::SourceApiV1);
            const auto results = discovery.results();
            QCOMPARE(results.size(), 1);
            QCOMPARE(results.first().serialNumber, QString("604415"));
            QCOMPARE(results.first().friendlyName, QString("TXT name"));
            QCOMPARE(results.first().firmwareVersion, QString("60.4"));
            QCOMPARE(results.first().manufacturer, QString("go-e"));
            QVERIFY(results.first().apiAvailableV1);
            QVERIFY(results.first().apiAvailableV2);
        }
    }

    void httpOnlyAndV1()
    {
        for (bool v2 : {false, true}) {
            GoeDiscoveryData::Results<GoeDiscovery::Result> discovery;
            GoeDiscovery::Result http;
            http.address = QHostAddress("192.0.2.1");
            http.serialNumber = "604415";
            http.apiAvailableV1 = !v2;
            http.apiAvailableV2 = v2;
            discovery.add(http, v2 ? GoeDiscoveryData::SourceApiV2 : GoeDiscoveryData::SourceApiV1);
            QCOMPARE(discovery.results().size(), 1);
            QCOMPARE(discovery.results().first().product, QString("go-eCharger"));
        }
        GoeDiscoveryData::Results<GoeDiscovery::Result> discovery;
        GoeDiscovery::Result unverified;
        unverified.serialNumber = "604415";
        discovery.add(unverified, GoeDiscoveryData::SourceZeroConf);
        QVERIFY(discovery.results().isEmpty());
    }

    void optionalFields()
    {
        for (const QString &firmware : {QString("60.4"), QString("51.1"), QString("unknown")}) {
            QVariantMap status;
            for (const QString &field : GoeStatus::pollingFields())
                status.insert(field, 1);
            status["fwv"] = firmware;
            QCOMPARE(GoeStatus::supportedFields(GoeStatus::pollingFields(), status), GoeStatus::pollingFields());
            for (const QString &optional : {QString("ast"), QString("upd"), QString("cbl")})
                status.remove(optional);
            const auto supported = GoeStatus::supportedFields(GoeStatus::pollingFields(), status);
            QCOMPARE(supported.size(), GoeStatus::pollingFields().size() - 3);
            QVERIFY(supported.contains("ama"));
            QVERIFY(!supported.contains("cbl"));
            status["cbl"] = 16;
            QVERIFY(GoeStatus::supportedFields(GoeStatus::pollingFields(), status).contains("cbl"));
        }
    }

    void httpResponses_data()
    {
        QTest::addColumn<int>("code");
        QTest::addColumn<QByteArray>("body");
        QTest::addColumn<bool>("accepted");
        const QByteArray payload = R"({"alw":true,"car":1,"tma":[null,null,24.375,26.5,26.5,24.5],"eto":450,"wh":440,"fwv":"60.4","amp":16,"adi":false,"fhz":49.96300125,"ama":16,"var":22,"pnp":0,"nrg":[235.8670044,234.0390015,236.5740051,0.024,0,0,0,0,0,0,0,0,0,0,0,0],"pha":[false,false,false,true,true,true]})";
        QTest::newRow("empty200") << 200 << QByteArray("{}") << false;
        QTest::newRow("error200") << 200 << QByteArray("{\"error\":\"bad request\"}") << false;
        QTest::newRow("boot200") << 200 << QByteArray("{\"alw\":true,\"car\":null,\"nrg\":[]}") << true;
        QTest::newRow("boot400") << 400 << QByteArray("{\"alw\":true,\"car\":null,\"nrg\":[]}") << false;
        QTest::newRow("partial400") << 400 << payload << true;
        QTest::newRow("normal200") << 200 << payload << true;
        QTest::newRow("serverError") << 500 << payload << false;
        QTest::newRow("notFound") << 404 << payload << false;
        QTest::newRow("malformed") << 400 << QByteArray("{") << false;
        QTest::newRow("empty") << 400 << QByteArray("{}") << false;
        QTest::newRow("error") << 400 << QByteArray("{\"error\":\"bad request\"}") << false;
    }

    void httpResponses()
    {
        QFETCH(int, code);
        QFETCH(QByteArray, body);
        QFETCH(bool, accepted);
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        connect(&server, &QTcpServer::newConnection, this, [&]() {
            auto socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [=]() {
                socket->readAll();
                socket->write("HTTP/1.1 " + QByteArray::number(code) + " Response\r\nContent-Type: application/json\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        });
        QNetworkAccessManager manager;
        auto reply = manager.get(QNetworkRequest(QUrl(QString("http://127.0.0.1:%1/api/status").arg(server.serverPort()))));
        QSignalSpy finished(reply, &QNetworkReply::finished);
        QVERIFY(finished.wait());
        QCOMPARE(GoeStatus::acceptableReply(reply, true), accepted);
        if (code != 200)
            QVERIFY(!GoeStatus::acceptableReply(reply, false));
        QCOMPARE(reply->readAll(), body); // Validation must not consume the payload.
        reply->deleteLater();
    }
};
QTEST_GUILESS_MAIN(TestGoeCompatibility)
#include "tst_goecompatibility.moc"
