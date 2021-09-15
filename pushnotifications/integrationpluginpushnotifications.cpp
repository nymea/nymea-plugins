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

#include "integrationpluginpushnotifications.h"
#include "plugininfo.h"

#include "network/networkaccessmanager.h"
#include "nymeasettings.h"

#include <QJsonDocument>

// Example payload for Firebase + GCM
//{
//    "android": {
//        "notification": {
//            "sound": "default"
//        },
//        "priority": "high"
//    },
//    "data": {
//        "body": "text",
//        "title": "title"
//    },
//    "to": "<client token>"
//}


// Example payload for Firebase + APNs
//{
//    "apns": {
//        "headers": {
//            "apns-priority": "10"
//        }
//    },
//    "notification": {
//        "body": "text",
//        "sound": "default",
//        "title": "title"
//    },
//    "to": "<client token>"
//}


IntegrationPluginPushNotifications::IntegrationPluginPushNotifications(QObject* parent): IntegrationPlugin (parent)
{
}

IntegrationPluginPushNotifications::~IntegrationPluginPushNotifications()
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
    if (pushService.startsWith("FB") && apiKeyStorage()->requestKey("firebase").data("apiKey").isEmpty()) {
        //: Error setting up thing
        info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Firebase server API key not installed."));
        return;
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

    if (pushService != "None" && token.isEmpty()) {
        return info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Push notifications need to be reconfigured."));
    }

    QVariantMap nymeaData;
    // FIXME: This is quite ugly but there isn't an API that allows to retrieve the server UUID yet
    NymeaSettings settings(NymeaSettings::SettingsRoleGlobal);
    settings.beginGroup("nymead");
    QUuid serverUuid = settings.value("uuid").toUuid();
    settings.endGroup();
    nymeaData.insert("serverUuid", serverUuid);
    nymeaData.insert("data", data);

    QNetworkRequest request;
    QVariantMap payload;

    if (pushService.startsWith("FB")) {

        ApiKey apiKey = apiKeyStorage()->requestKey("firebase");
        if (apiKey.data("apiKey").isEmpty()) {
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("Firebase server API key not installed."));
            return;
        }

        request = QNetworkRequest(QUrl("https://fcm.googleapis.com/fcm/send"));
        request.setRawHeader("Authorization", "key=" + apiKey.data("apiKey"));
        request.setRawHeader("Content-Type", "application/json");

        payload.insert("to", token.toUtf8().trimmed());

        QVariantMap notification;
        notification.insert("title", title);
        notification.insert("body", body);
        notification.insert("nymeaData", nymeaData);


        if (pushService == "FB-GCM") {

            QVariantMap soundMap;
            soundMap.insert("sound", "default");

            QVariantMap android;
            android.insert("priority", "high");
            android.insert("notification", soundMap);

            payload.insert("android", android);
            payload.insert("data", notification);

        } else if (pushService == "FB-APNs") {

            notification.insert("sound", "default");

            QVariantMap headers;
            headers.insert("apns-priority", "10");
            QVariantMap apns;
            apns.insert("headers", headers);

            payload.insert("notification", notification);
            payload.insert("apns", apns);
        }


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
        notification.insert("vibrate", true);
        notification.insert("sound", true);
        notification.insert("nymeaData", nymeaData);

        QVariantMap data;
        data.insert("notification", notification);

        payload.insert("data", data);
        payload.insert("appid", "io.guh.nymeaapp_nymea-app");
        payload.insert("expire_on", QDateTime::currentDateTime().toUTC().addMSecs(1000 * 60 * 10).toString(Qt::ISODate));
        payload.insert("token", token.toUtf8().trimmed());
    } else if (pushService == "None") {
        // Nothing to do here... It's the clients responsibility to fetch it from nymea
        info->finish(Thing::ThingErrorNoError);
        return;
    }

    qCDebug(dcPushNotifications()) << "Sending notification" << request.url().toString() << qUtf8Printable(QJsonDocument::fromVariant(payload).toJson());
    QNetworkReply *reply = hardwareManager()->networkManager()->post(request, QJsonDocument::fromVariant(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [reply, pushService, info, this]{
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcPushNotifications()) << "Push message sending failed for" << info->thing()->name() << info->thing()->id() << reply->errorString() << reply->error();
            emit info->finish(Thing::ThingErrorHardwareNotAvailable);
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
//        qDebug(dcPushNotifications) << qUtf8Printable(jsonDoc.toJson());

        if (pushService == "FB-GCM" || pushService == "FB-APNs") {
            if (replyMap.value("success").toInt() != 1) {

                // While GCM seems rock solid, APNs fails rather often with Internal Server Error.
                // According to Firebase support this is "expected" and one should retry with a exponential back-off timer.
                // As we only have 30 secs until the info times out, let's try repeatedly until the info object dies.
                // In my tests, so far it succeeded every time on the second attempt.
                // https://stackoverflow.com/questions/63382257/firebase-messaging-fails-sporadically-with-internal-error
                if (replyMap.value("results").toList().count() > 0 && replyMap.value("results").toList().first().toMap().value("error").toString() == "InternalServerError") {
                    qCDebug(dcPushNotifications()) << "Sending push message failed. Retrying...";
                    executeAction(info);
                    return;
                }

                // On any other error, bail out...
                qCWarning(dcPushNotifications()) << "Error sending push notification:" << qUtf8Printable(jsonDoc.toJson());
                info->finish(Thing::ThingErrorHardwareFailure);
                return;
            }
        } else if (pushService == "UBPorts") {
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

