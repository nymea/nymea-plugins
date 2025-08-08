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

#include "integrationpluginpushnotifications.h"
#include "plugininfo.h"

#include <network/networkaccessmanager.h>
#include <nymeasettings.h>

#include <QJsonDocument>

IntegrationPluginPushNotifications::IntegrationPluginPushNotifications(QObject *parent)
    : IntegrationPlugin{parent}
{
}

IntegrationPluginPushNotifications::~IntegrationPluginPushNotifications()
{

}

void IntegrationPluginPushNotifications::init()
{

}

void IntegrationPluginPushNotifications::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    QString token = thing->paramValue(pushNotificationsThingTokenParamTypeId).toString();
    QString pushService = thing->paramValue(pushNotificationsThingServiceParamTypeId).toString();
    QString clientId = thing->paramValue(pushNotificationsThingClientIdParamTypeId).toString();

    qCDebug(dcPushNotifications()) << "Setting up push notifications" << thing->name() << "(" << clientId << ") for service" << pushService << "with token" << (token.mid(0, 5) + "******");

    QStringList availablePushServices = {"FB-GCM", "FB-APNs", "UBPorts", "None"};
    if (!availablePushServices.contains(pushService)) {
        //: Error setting up thing
        info->finish(Thing::ThingErrorMissingParameter, QT_TR_NOOP("The push service must not be empty."));
        return;
    }

    if (pushService != "None" && token.isEmpty()) {
        //: Error setting up thing
        info->finish(Thing::ThingErrorMissingParameter, QT_TR_NOOP("The token must not be empty."));
        return;
    }

    // In case of Firebase, check if we have the required API key
    if (pushService.startsWith("FB")) {
        ApiKey apiKey = apiKeyStorage()->requestKey("firebase");
        if (apiKey.data("project_id").isEmpty() || apiKey.data("private_key_id").isEmpty() ||
            apiKey.data("private_key").isEmpty() || apiKey.data("client_email").isEmpty()) {
            //: Error setting up thing
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Firebase server API key not installed."));
            return;
        }
    }

    if (!m_google) {
        qCDebug(dcPushNotifications()) << "Creating google OAuth2 client...";
        m_google = new GoogleOAuth2(hardwareManager()->networkManager(), apiKeyStorage()->requestKey("firebase"), this);
        m_google->authorize();
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginPushNotifications::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcPushNotifications()) << "Executing action" << action.actionTypeId() << "for" << thing->name() << thing->id().toString();

    QString token = thing->paramValue(pushNotificationsThingTokenParamTypeId).toString();
    QString pushService = thing->paramValue(pushNotificationsThingServiceParamTypeId).toString();
    QString title = action.param(pushNotificationsNotifyActionTitleParamTypeId).value().toString();
    QString body = action.param(pushNotificationsNotifyActionBodyParamTypeId).value().toString();
    QString data = action.paramValue(pushNotificationsNotifyActionDataParamTypeId).toString();
    QString notificationId = action.paramValue(pushNotificationsNotifyActionNotificationIdParamTypeId).toString();
    bool sound = action.paramValue(pushNotificationsNotifyActionSoundParamTypeId).toBool();

    if (pushService != "None" && token.isEmpty()) {
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Push notifications need to be reconfigured."));
        return;
    }

    if (notificationId.isEmpty()) {
        notificationId = QUuid::createUuid().toString();
    }

    // FIXME: This is quite ugly but there isn't an API that allows to retrieve the server UUID yet
    NymeaSettings settings(NymeaSettings::SettingsRoleGlobal);
    settings.beginGroup("nymead");
    QUuid serverUuid = settings.value("uuid").toUuid();
    settings.endGroup();

    QVariantMap nymeaData;
    nymeaData.insert("serverUuid", serverUuid);
    nymeaData.insert("data", data);

    QNetworkRequest request;
    QVariantMap payload;

    if (pushService.startsWith("FB")) {

        if (!m_google->authenticated()) {
            qCWarning(dcPushNotifications()) << "Google OAUth2 client is not authorized. Retry autorizing ...";
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Cannot access notification service. Please try again later."));
            m_google->authorize();
            return;
        }

        ApiKey apiKey = apiKeyStorage()->requestKey("firebase");
        request = QNetworkRequest(QUrl(QString("https://fcm.googleapis.com/v1/projects/%1/messages:send").arg(QString::fromUtf8(apiKey.data("project_id")))));
        request.setRawHeader("Authorization", "Bearer " + m_google->accessToken().toUtf8());
        request.setRawHeader("Content-Type", "application/json");

        // https://firebase.google.com/docs/reference/fcm/rest/v1/projects.messages

        QVariantMap message;
        message.insert("token", token.toUtf8().trimmed());
        QVariantMap notification;
        notification.insert("title", title);
        notification.insert("body", body);
        message.insert("notification", notification);

        // The data map must be a Map<String, String>
        QVariantMap dataMap;
        dataMap.insert("nymeaData", QString::fromUtf8(QJsonDocument::fromVariant(nymeaData).toJson(QJsonDocument::Compact)));
        dataMap.insert("sound", sound ? "true" : "false");
        message.insert("data", dataMap);

        if (pushService == "FB-GCM") {

            QVariantMap android;
            android.insert("priority", "high");

            message.insert("android", android);

        } else if (pushService == "FB-APNs") {

            QVariantMap headers;
            headers.insert("apns-priority", "10");

            QVariantMap apns;
            apns.insert("headers", headers);

            message.insert("apns", apns);
        }

        payload.insert("message", message);

    } else if (pushService == "UBPorts") {
        request = QNetworkRequest(QUrl("https://push.ubports.com/notify"));
        request.setRawHeader("Content-Type", "application/json");

        QVariantMap card;
        card.insert("icon", "notification");
        card.insert("summary", title);
        card.insert("body", body);
        card.insert("popup", true);
        card.insert("persist", true);

        QVariantMap notification;
        notification.insert("card", card);
        notification.insert("vibrate", sound);
        notification.insert("sound", sound);
        notification.insert("nymeaData", nymeaData);

        QVariantMap data;
        data.insert("notification", notification);

        payload.insert("data", data);
        payload.insert("appid", "io.guh.nymeaapp_nymea-app");
        payload.insert("expire_on", QDateTime::currentDateTimeUtc().addMSecs(1000 * 60 * 10).toString(Qt::ISODate));
        payload.insert("token", token.toUtf8().trimmed());
    } else if (pushService == "None") {
        // Nothing to do here... It's the clients responsibility to fetch it from nymea
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    qCDebug(dcPushNotifications()) << "Sending notification" << request.url().toString() << qUtf8Printable(QJsonDocument::fromVariant(payload).toJson());
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [reply, pushService, info]{

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPushNotifications()) << "Push message sending failed for" << info->thing()->name() << info->thing()->id() << reply->errorString() << reply->error();
            info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcPushNotifications()) << "Error reading reply from server for" << info->thing()->name() << info->thing()->id().toString() << error.errorString();
            qCWarning(dcPushNotifications()) << qUtf8Printable(data);
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        QVariantMap replyMap = jsonDoc.toVariant().toMap();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        qCDebug(dcPushNotifications()) << status << qUtf8Printable(jsonDoc.toJson());

        if (pushService == "UBPorts") {
            if (!replyMap.value("ok").toBool()) {
                qCWarning(dcPushNotifications()) << "Error sending push notification:" << qUtf8Printable(jsonDoc.toJson());
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }
        }

        qCDebug(dcPushNotifications()) << "Message sent successfully";
        info->finish(Thing::ThingErrorNoError);
    });
}

