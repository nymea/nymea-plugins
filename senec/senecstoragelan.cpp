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

#include "senecstoragelan.h"
#include "extern-plugininfo.h"

#include <QDataStream>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkRequest>

SenecStorageLan::SenecStorageLan(NetworkAccessManager *networkManager, QObject *parent)
    : QObject{parent},
    m_networkManager{networkManager}
{

}

SenecStorageLan::SenecStorageLan(NetworkAccessManager *networkManager, const QHostAddress &address, QObject *parent)
    : QObject{parent},
    m_networkManager{networkManager},
    m_address{address}
{
    updateUrl();
}

QHostAddress SenecStorageLan::address() const
{
    return m_address;
}

void SenecStorageLan::setAddress(const QHostAddress &address)
{
    m_address = address;
    updateUrl();
}

QUrl SenecStorageLan::url() const
{
    return m_url;
}

bool SenecStorageLan::available() const
{
    return m_available;
}

QString SenecStorageLan::deviceId() const
{
    return m_deviceId;
}

float SenecStorageLan::capacity() const
{
    return m_capacity;
}

float SenecStorageLan::maxChargePower() const
{
    return m_maxChargePower;
}

float SenecStorageLan::maxDischargePower() const
{
    return m_maxDischargePower;
}

float SenecStorageLan::batteryLevel() const
{
    return m_batteryLevel;
}

float SenecStorageLan::batteryPower() const
{
    return m_batteryPower;
}

float SenecStorageLan::gridPower() const
{
    return m_gridPower;
}

float SenecStorageLan::inverterPower() const
{
    return m_inverterPower;
}

float SenecStorageLan::parseFloat(const QString &value)
{
    Q_ASSERT_X(value.left(3) == "fl_", "SenecStorageLan", "The given value does not seem to be a float, it is not starting with fl_");
    QString hexString = value.right(value.length() - 3);

    float result = 0;
    quint32 dataValue = 0;

    QByteArray rawData = QByteArray::fromHex(hexString.toUtf8());
    Q_ASSERT_X(rawData.count() == 4,  "ModbusDataUtils", "invalid raw data size for converting value to float32. The rawdate has not the expected size of 4.");
    QDataStream stream(rawData);
    stream >> dataValue;
    memcpy(&result, &dataValue, sizeof(quint32));
    return result;
}

QString SenecStorageLan::parseString(const QString &value)
{
    Q_ASSERT_X(value.left(3) == "st_", "SenecStorageLan", "The given value does not seem to be a string, it is not starting with st_");
    return value.right(value.length() - 3);
}

// quint8 SenecStorageLan::parseUInt8(const QString &value)
// {
//     Q_ASSERT_X(value.left(3) == "u8_", "SenecStorageLan", "The given value does not seem to be a uint8, it is not starting with u8_");

// }


void SenecStorageLan::initialize()
{
    // if (m_url.isValid()) {
    //     qCWarning(dcSenec()) << "Cannot initialize the storage. The request URL is not valid. Maybe the IP is not known yet or invalid.";
    //     emit initializeFinished(false);
    //     return;
    // }

    // Verify the debug request is working
    QVariantMap requestData;
    requestData.insert("DEBUG", QVariantMap());

    m_url = QUrl(QString("https://%1/lala.cgi").arg(m_address.toString()));

    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray data = QJsonDocument::fromVariant(requestData).toJson(QJsonDocument::Compact);

    qCDebug(dcSenec()) << "POST" << m_url.toString() << m_address.toString() << qUtf8Printable(data);

    QNetworkReply *reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::sslErrors, this, &SenecStorageLan::ignoreSslErrors);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSenec()) << "Debug request finished with error. Status:" << status << "Error:" << reply->errorString();
            setAvailable(false);
            emit initializeFinished(false);
            return;
        }

        QByteArray responseData = reply->readAll();

        QJsonParseError jsonError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &jsonError);
        QVariantMap responseMap = jsonDoc.toVariant().toMap();
        if (jsonError.error != QJsonParseError::NoError) {
            qCWarning(dcSenec()) << "Debug request finished successfully, but the response contains invalid JSON object:" << responseData;
            setAvailable(false);
            emit initializeFinished(false);
            return;
        }

        // Verify content, if the content is valid, we assume we have found a SENEC storage
        if (!responseMap.contains("DEBUG")) {
            qCWarning(dcSenec()) << "Unexpected reponse data from debug request. Aborting...";
            setAvailable(false);
            emit initializeFinished(false);
            return;
        }

        qCDebug(dcSenec()) << "Debug request finished successfully" << qUtf8Printable(jsonDoc.toJson());

        // Request basic information

        QVariantMap requestData;
        QVariantMap factoryMap;
        factoryMap.insert("DEVICE_ID", QString());
        factoryMap.insert("DESIGN_CAPACITY", QString());
        factoryMap.insert("MAX_CHARGE_POWER_DC", QString());
        factoryMap.insert("MAX_DISCHARGE_POWER_DC", QString());
        requestData.insert("FACTORY", factoryMap);

        QNetworkRequest request(m_url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply *reply = m_networkManager->post(QNetworkRequest(m_url), QJsonDocument::fromVariant(requestData).toJson());
        connect(reply, &QNetworkReply::sslErrors, this, &SenecStorageLan::ignoreSslErrors);
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
        connect(reply, &QNetworkReply::finished, this, [this, reply] {


            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            // Check HTTP status code
            if (status != 200 || reply->error() != QNetworkReply::NoError) {
                qCWarning(dcSenec()) << "Factory request finished with error. Status:" << status << "Error:" << reply->errorString();
                setAvailable(false);
                emit initializeFinished(false);
                return;
            }

            QByteArray responseData = reply->readAll();

            QJsonParseError jsonError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &jsonError);
            QVariantMap responseMap = jsonDoc.toVariant().toMap();
            if (jsonError.error != QJsonParseError::NoError) {
                qCWarning(dcSenec()) << "Factory request finished successfully, but the response contains invalid JSON object:" << responseData;
                setAvailable(false);
                emit initializeFinished(false);
                return;
            }

            qCDebug(dcSenec()) << "Factory request finished successfully" << qUtf8Printable(jsonDoc.toJson());

            QVariantMap factoryResponseMap = responseMap.value("FACTORY").toMap();
            m_deviceId = parseString(factoryResponseMap.value("DEVICE_ID").toString());
            m_capacity = parseFloat(factoryResponseMap.value("DESIGN_CAPACITY").toString());
            m_maxChargePower = parseFloat(factoryResponseMap.value("MAX_CHARGE_POWER_DC").toString());
            m_maxDischargePower = parseFloat(factoryResponseMap.value("MAX_DISCHARGE_POWER_DC").toString());

            emit initializeFinished(true);
            setAvailable(true);

            qCDebug(dcSenec()) << "Initialized successfully";
        });
    });
}

void SenecStorageLan::update()
{
    // if (m_url.isValid()) {
    //     qCDebug(dcSenec()) << "Cannot update the storage. The request URL is not valid. Maybe the IP is not known yet or invalid.";
    //     emit updatedFinished(false);
    //     return;
    // }

    QVariantMap requestData;

    QVariantMap energyMap;
    energyMap.insert("GUI_BAT_DATA_POWER", QString());
    energyMap.insert("GUI_INVERTER_POWER", QString());
    energyMap.insert("GUI_BAT_DATA_FUEL_CHARGE", QString());
    energyMap.insert("GUI_HOUSE_POW", QString());
    energyMap.insert("GUI_GRID_POW", QString());

    requestData.insert("ENERGY", energyMap);

    m_url = QUrl(QString("https://%1/lala.cgi").arg(m_address.toString()));

    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument::fromVariant(requestData).toJson());
    connect(reply, &QNetworkReply::sslErrors, this, &SenecStorageLan::ignoreSslErrors);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {

        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // Check HTTP status code
        if (status != 200 || reply->error() != QNetworkReply::NoError) {
            qCWarning(dcSenec()) << "Update request finished with error. Status:" << status << "Error:" << reply->errorString();
            setAvailable(false);
            emit updatedFinished(false);
            return;
        }

        QByteArray responseData = reply->readAll();

        QJsonParseError jsonError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &jsonError);
        QVariantMap responseMap = jsonDoc.toVariant().toMap();
        if (jsonError.error != QJsonParseError::NoError) {
            qCWarning(dcSenec()) << "Update request finished successfully, but the response contains invalid JSON object:" << responseData;
            setAvailable(false);
            emit updatedFinished(false);
            return;
        }

        qCDebug(dcSenec()) << "Update request finished successfully" << qUtf8Printable(jsonDoc.toJson());

        QVariantMap energyResponseMap = responseMap.value("ENERGY").toMap();
        m_batteryPower = parseFloat(energyResponseMap.value("GUI_BAT_DATA_POWER").toString());
        m_batteryLevel = parseFloat(energyResponseMap.value("GUI_BAT_DATA_FUEL_CHARGE").toString());
        m_gridPower = parseFloat(energyResponseMap.value("GUI_GRID_POW").toString());
        m_inverterPower = -1* parseFloat(energyResponseMap.value("GUI_INVERTER_POWER").toString());

        setAvailable(true);

        qCDebug(dcSenec()).nospace().noquote() << "Update values: Battery power: " << m_batteryPower
                                               << "W, Battery level: " << m_batteryLevel
                                               << "%, Grid power: " << m_gridPower
                                               << "W, Inverter power: " << m_inverterPower << "W";

        emit updatedFinished(true);
    });
}

void SenecStorageLan::updateUrl()
{
    m_url = QUrl(QString("https://%1/lala.cgi").arg(m_address.toString()));
}

void SenecStorageLan::setAvailable(bool available)
{
    if (m_available == available)
        return;

    m_available = available;
    emit availableChanged(m_available);
}

void SenecStorageLan::ignoreSslErrors(const QList<QSslError> &errors)
{
    qCDebug(dcSenec()) << "SSL errors occurred" << errors << "but we are ignoring them in the LAN environment...";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->ignoreSslErrors();
}
