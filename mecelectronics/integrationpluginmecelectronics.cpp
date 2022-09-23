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

#include "integrationpluginmecelectronics.h"
#include "plugininfo.h"

#include <network/networkaccessmanager.h>
#include <plugintimer.h>
#include <platform/platformzeroconfcontroller.h>
#include <network/zeroconf/zeroconfservicebrowser.h>

#include <QNetworkReply>
#include <QJsonDocument>
#include <QTimer>


IntegrationPluginMecMeter::IntegrationPluginMecMeter()
{
}

IntegrationPluginMecMeter::~IntegrationPluginMecMeter()
{
}

void IntegrationPluginMecMeter::init()
{
    m_zeroConf = hardwareManager()->zeroConfController()->createServiceBrowser("_http._tcp");

    connect(m_zeroConf, &ZeroConfServiceBrowser::serviceEntryAdded, this, [=](const ZeroConfServiceEntry &entry){
        if (myThings().findByParams({Param(mecMeterThingIdParamTypeId, entry.name())})) {
            pluginStorage()->beginGroup(entry.name());
            pluginStorage()->setValue("cachedAddress", entry.hostAddress().toString());
            pluginStorage()->endGroup();
        }
    });
}

void IntegrationPluginMecMeter::discoverThings(ThingDiscoveryInfo *info)
{
    foreach (const ZeroConfServiceEntry &entry, m_zeroConf->serviceEntries()) {
        if (entry.protocol() != QAbstractSocket::IPv4Protocol) {
            continue;
        }
        qCDebug(dcMecElectronics()) << "zeroconf entry:" << entry;
        QRegExp match("mec[A-Z0-9]{12}");
        if (match.exactMatch(entry.name())) {
            qCDebug(dcMecElectronics()) << "Found mec meter!";
            ThingDescriptor descriptor(mecMeterThingClassId, entry.name(), entry.hostAddress().toString());
            descriptor.setParams({Param(mecMeterThingIdParamTypeId, entry.name())});
            info->addThingDescriptor(descriptor);
        }
    }
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginMecMeter::startPairing(ThingPairingInfo *info)
{
    info->finish(Thing::ThingErrorNoError, QT_TR_NOOP("Please enter your login credentials for the mecMeter."));
}

void IntegrationPluginMecMeter::confirmPairing(ThingPairingInfo *info, const QString &username, const QString &secret)
{
    QString meterId = info->params().paramValue(mecMeterThingIdParamTypeId).toString();
    QNetworkRequest request = composeRequest(meterId, username, secret);
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, info, [=](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcMecElectronics()) << "Error connecting to mecmeter:" << reply->error() << reply->errorString();
            // Device responds with InternalServerError on wrong login
            if (reply->error() == QNetworkReply::InternalServerError) {
                info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The login credentials are not valid."));
                return;
            }
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        pluginStorage()->beginGroup(meterId);
        pluginStorage()->setValue("username", username);
        pluginStorage()->setValue("password", secret);
        pluginStorage()->endGroup();

        info->finish(Thing::ThingErrorNoError);
    });

}

void IntegrationPluginMecMeter::setupThing(ThingSetupInfo *info)
{
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginMecMeter::postSetupThing(Thing *thing)
{
    Q_UNUSED(thing)

    if (!m_timer) {
        m_timer = hardwareManager()->pluginTimerManager()->registerTimer(1);
        connect(m_timer, &PluginTimer::timeout, this, [this](){
            foreach (Thing *thing, myThings()) {
                refresh(thing);
            }
        });
    }
}

void IntegrationPluginMecMeter::thingRemoved(Thing *thing)
{
    Q_UNUSED(thing)
    if (myThings().isEmpty() && m_timer) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_timer);
        m_timer = nullptr;
    }
}

void IntegrationPluginMecMeter::refresh(Thing *thing)
{
    QString meterId = thing->paramValue(mecMeterThingIdParamTypeId).toString();
    pluginStorage()->beginGroup(meterId);
    QString username = pluginStorage()->value("username").toString();
    QString password = pluginStorage()->value("password").toString();
    pluginStorage()->endGroup();
    QNetworkRequest request = composeRequest(thing->paramValue(mecMeterThingIdParamTypeId).toString(), username, password);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, thing, [thing, reply](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcMecElectronics()) << "Failed to refresh meter data. The reply returned with error" << reply->errorString();
            thing->setStateValue(mecMeterConnectedStateTypeId, false);
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcMecElectronics()) << "Failed to parse meter data" << data << ":" << error.errorString();
            return;
        }

        thing->setStateValue(mecMeterConnectedStateTypeId, true);

        QVariantMap dataMap = jsonDoc.toVariant().toMap();
        qCDebug(dcMecElectronics()) << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));

        /*
         {
            "EFAA": 0,
            "EFAB": 0,
            "EFAC": 0,
            "EFAF": 0,
            "EFAH": 0,
            "EFAT": 0,
            "EFBF": 0,
            "EFBH": 0,
            "EFCF": 0,
            "EFCH": 0,
            "EFRA": 0,
            "EFRB": 0,
            "EFRC": 0.026041666666666668,
            "EFRT": 0.026041666666666668,
            "EFSA": 0,
            "EFSB": 0,
            "EFSC": 1193.7760416666667,
            "EFST": 841.9010416666667,
            "EFTF": 0,
            "EFTH": 0,
            "EMT": 1.9661458333333335,
            "ERA1": 0,
            "ERA2": 0,
            "ERA3": 0,
            "ERA4": 0,
            "ERAA": 0,
            "ERAB": 0,
            "ERAC": 11.848958333333334,
            "ERAF": 0,
            "ERAH": 0,
            "ERAT": 11.848958333333334,
            "ERB1": 0,
            "ERB2": 0,
            "ERB3": 0,
            "ERB4": 0,
            "ERBF": 0,
            "ERBH": 0,
            "ERC1": 0,
            "ERC2": 0.026041666666666668,
            "ERC3": 1.7447916666666667,
            "ERC4": 0.9895833333333334,
            "ERCF": 11.822916666666668,
            "ERCH": 0,
            "ERRA": 0,
            "ERRB": 0,
            "ERRC": 2.734375,
            "ERRT": 2.734375,
            "ERSA": 0,
            "ERSB": 0,
            "ERSC": 2341.588541666667,
            "ERST": 2693.463541666667,
            "ERT1": 0,
            "ERT2": 0.026041666666666668,
            "ERT3": 2.265625,
            "ERT4": 0.46875,
            "ERTF": 11.822916666666668,
            "ERTH": 0,
            "ESA": 0,
            "ESB": 0,
            "ESC": 3535.3645833333335,
            "EST": 3535.3645833333335,
            "EVT": 2355.7552083333335,
            "F": 49.99,
            "IA": 0.007428385416666667,
            "IAA": 0,
            "IAB": 0,
            "IAC": 30.200000000000003,
            "IADC": -6.40869140625e-05,
            "IB": 0.03445963541666667,
            "IBDC": -0.00044403076171874997,
            "IC": 0.012877604166666667,
            "ICDC": 0.000157928466796875,
            "IN": 3.429166666666667,
            "IN0": 0.034,
            "PA": 0,
            "PAF": 0,
            "PAH": 0,
            "PB": 0,
            "PBF": 0,
            "PBH": 0,
            "PC": -0.01953125,
            "PCF": -0.016276041666666668,
            "PCH": 0,
            "PFA": 0,
            "PFB": 0,
            "PFC": -0.008,
            "PFT": -0.007,
            "PT": -0.013020833333333334,
            "PTF": -0.013020833333333334,
            "PTH": 0,
            "QA": 0,
            "QB": 0,
            "QC": -0.009765625,
            "QT": 0,
            "SA": 0,
            "SAMPLES": 13385877,
            "SB": 0,
            "SC": 2.98828125,
            "ST": 2.9817708333333335,
            "STATUS": 372,
            "T": 36,
            "THIA": 0,
            "THIB": 0,
            "THIC": 40.81666666666667,
            "THUA": 0,
            "THUB": 0,
            "THUC": 2.5250000000000004,
            "TIME": 4283480878,
            "UAA": 0,
            "UAB": 0,
            "UAC": 0,
            "VA": 0,
            "VAB": 0,
            "VB": 0,
            "VBC": 231.947734375,
            "VC": 231.947734375,
            "VCA": 231.947734375,
            "VPT": 154.63182291666666,
            "VT": 77.31591145833333
        }
        */


        // Total energy / power
        thing->setStateValue(mecMeterTotalEnergyConsumedStateTypeId, 0.001 * qRound(dataMap.value("EFAT").toDouble()));
        thing->setStateValue(mecMeterTotalEnergyProducedStateTypeId, 0.001 * qRound(dataMap.value("ERAT").toDouble()));
        thing->setStateValue(mecMeterCurrentPowerStateTypeId, dataMap.value("PT").toDouble());
//        thing->setStateValue(mecMeterTotalForwardeReactiveEnergyStateTypeId, dataMap.value("EFRT").toDouble());

        // Voltage
        thing->setStateValue(mecMeterVoltagePhaseAStateTypeId, dataMap.value("VA").toDouble());
        thing->setStateValue(mecMeterVoltagePhaseBStateTypeId, dataMap.value("VB").toDouble());
        thing->setStateValue(mecMeterVoltagePhaseCStateTypeId, dataMap.value("VC").toDouble());

        // Current
        thing->setStateValue(mecMeterCurrentPhaseAStateTypeId, dataMap.value("IA").toDouble());
        thing->setStateValue(mecMeterCurrentPhaseBStateTypeId, dataMap.value("IB").toDouble());
        thing->setStateValue(mecMeterCurrentPhaseCStateTypeId, dataMap.value("IC").toDouble());

        // Power
        thing->setStateValue(mecMeterCurrentPowerPhaseAStateTypeId, dataMap.value("PA").toDouble());
        thing->setStateValue(mecMeterCurrentPowerPhaseBStateTypeId, dataMap.value("PB").toDouble());
        thing->setStateValue(mecMeterCurrentPowerPhaseCStateTypeId, dataMap.value("PC").toDouble());

        // Frequency
//        thing->setStateValue(mecMeterFrequencyStateTypeId, dataMap.value("F").toDouble());

        // Energy consumed
        thing->setStateValue(mecMeterEnergyConsumedPhaseAStateTypeId, 0.001 * qRound(dataMap.value("EFAA").toDouble()));
        thing->setStateValue(mecMeterEnergyConsumedPhaseBStateTypeId, 0.001 * qRound(dataMap.value("EFAB").toDouble()));
        thing->setStateValue(mecMeterEnergyConsumedPhaseCStateTypeId, 0.001 * qRound(dataMap.value("EFAC").toDouble()));

        // Energy produced
        thing->setStateValue(mecMeterEnergyProducedPhaseAStateTypeId, 0.001 * qRound(dataMap.value("ERAA").toDouble() / 1000.0));
        thing->setStateValue(mecMeterEnergyProducedPhaseBStateTypeId, 0.001 * qRound(dataMap.value("ERAB").toDouble() / 1000.0));
        thing->setStateValue(mecMeterEnergyProducedPhaseCStateTypeId, 0.001 * qRound(dataMap.value("ERAC").toDouble() / 1000.0));
    });
}

QNetworkRequest IntegrationPluginMecMeter::composeRequest(const QString &meterId, const QString &username, const QString &password)
{
    QHostAddress address;
    foreach (const ZeroConfServiceEntry &entry, m_zeroConf->serviceEntries()) {
        if (entry.protocol() == QAbstractSocket::IPv4Protocol && entry.name() == meterId) {
            address = entry.hostAddress();
            break;
        }
    }

    if (address.isNull()) {
        pluginStorage()->beginGroup(meterId);
        address = pluginStorage()->value("cachedAddress").toString();
        pluginStorage()->endGroup();
    }

    if (address.isNull()) {
        qCWarning(dcMecElectronics()) << "Error finding mecMeter device in the network";
        return QNetworkRequest();
    }

    QUrl url;
    url.setScheme("http");
    url.setHost(address.toString());
    url.setPath("/wizard/public/api/measurements");

    QNetworkRequest request(url);
    QString concatenated = username + ":" + password;
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());

    return request;
}

