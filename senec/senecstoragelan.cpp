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


void SenecStorageLan::initialize()
{
    if (m_url.isValid()) {
        qCWarning(dcSenec()) << "Cannot initialize the storage. The request URL is not valid. Maybe the IP is not known yet or invalid.";
        emit initializeFinished(false);
        return;
    }

    // Verify the debug request is working
    QVariantMap request;
    request.insert("DEBUG", QVariantMap());

    QNetworkReply *reply = m_networkManager->post(QNetworkRequest(m_url), QJsonDocument::fromVariant(request).toJson());
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

        QVariantMap request;
        QVariantMap factoryMap;
        factoryMap.insert("DEVICE_ID", QString());
        factoryMap.insert("DESIGN_CAPACITY", QString());
        factoryMap.insert("MAX_CHARGE_POWER_DC", QString());
        factoryMap.insert("MAX_DISCHARGE_POWER_DC", QString());
        request.insert("FACTORY", factoryMap);

        QNetworkReply *reply = m_networkManager->post(QNetworkRequest(m_url), QJsonDocument::fromVariant(request).toJson());
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

void SenecStorageLan::updateUrl()
{
    QUrl url;
    if (m_address.isNull()) {
        m_url = url;
        return;
    }

    url.setScheme("https");
    url.setHost(m_address.toString());
    url.setPath("lala.cgi");
    m_url = url;
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
