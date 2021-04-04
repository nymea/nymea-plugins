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

#include "integrationplugintelegram.h"
#include "plugininfo.h"

#include "network/networkaccessmanager.h"

#include <QJsonDocument>

IntegrationPluginTelegram::IntegrationPluginTelegram(QObject* parent): IntegrationPlugin (parent)
{

}

IntegrationPluginTelegram::~IntegrationPluginTelegram()
{

}

void IntegrationPluginTelegram::discoverThings(ThingDiscoveryInfo *info)
{
    QString token = info->params().paramValue(telegramDiscoveryTokenParamTypeId).toString().trimmed();
    QUrl url(QString("https://api.telegram.org/bot%1/getUpdates").arg(token));
    QNetworkRequest request(url);
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [=](){
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcTelegram()) << "Error fetching channels from telegram API:" << reply->error() << reply->errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to Telegram."));
            return;
        }
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTelegram()) << "Error parsing JSON reply from telegram API:" << error.error << error.errorString();
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Unexpected data from Telegram."));
            return;
        }
        if (!jsonDoc.toVariant().toMap().value("ok").toBool()) {
            qCWarning(dcTelegram()) << "Json reply doesn't contain OK" << qUtf8Printable(jsonDoc.toJson(QJsonDocument::Indented));
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("An error happened on the Telegram servers."));
            return;
        }
        qCDebug(dcTelegram()) << "Discovery data:" << qUtf8Printable(jsonDoc.toJson());

        QVariantList entries = jsonDoc.toVariant().toMap().value("result").toList();
        QList<int> addedChats;
        foreach (const QVariant &entry, entries) {
            QVariantMap entryMap = entry.toMap();
            QVariantMap chatMap;
            if (entryMap.contains("message")) {
                chatMap = entry.toMap().value("message").toMap().value("chat").toMap();
            } else if (entryMap.contains("my_chat_member")) {
                chatMap = entry.toMap().value("my_chat_member").toMap().value("chat").toMap();
            } else {
                qCWarning(dcTelegram()) << "Neither message nor my_chat_member found in entry. Skipping:" << qUtf8Printable(QJsonDocument::fromVariant(entryMap).toJson());
                continue;
            }
            int chatId = chatMap.value("id").toInt();
            if (addedChats.contains(chatId)) {
                qCDebug(dcTelegram()) << "Skipping chat" << chatId << "(Already added to discovery results)";
                continue;
            }
            QString chatName = QString("%1 %2")
                    .arg(chatMap.value("first_name").toString())
                    .arg(chatMap.value("last_name").toString());
            QString type = chatMap.value("type").toString();
            if (type == "group") {
                chatName = chatMap.value("title").toString();
            }
            ThingDescriptor descriptor(telegramThingClassId, chatName, type == "group" ? "Group" : "Private");
            ParamList params;
            params << Param(telegramThingTokenParamTypeId, token);
            params << Param(telegramThingChatIdParamTypeId, chatId);
            descriptor.setParams(params);

            Thing *existingThing = myThings().findByParams(params);
            if (existingThing) {
                qCDebug(dcTelegram()) << "Chat id" << chatId << "is already added as a thing.";
                descriptor.setThingId(existingThing->id());
            }
            addedChats.append(chatId);
            qCDebug(dcTelegram()) << "Adding chat" << chatId << descriptor.title() << descriptor.description();
            info->addThingDescriptor(descriptor);
        }
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginTelegram::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    qCDebug(dcTelegram()) << "Setting up telegram chat" << thing->name() << thing->id().toString();

    QString token = info->thing()->paramValue(telegramThingTokenParamTypeId).toString();
//    int chatId = info->thing()->paramValue(telegramThingChatIdParamTypeId).toInt();

    QString url = QString("https://api.telegram.org/bot%1/getMe").arg(token);
    QNetworkRequest  request(url);
    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);

    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, info, [reply, info]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcTelegram()) << "Error fetching user profile:" << reply->errorString() << reply->error();
            //: Error setting up thing
            info->finish(Thing::ThingErrorHardwareFailure, QT_TR_NOOP("Error connecting to Telegram."));
            return;
        }

        qCDebug(dcTelegram()) << "Telegram" << info->thing()->name() << info->thing()->id().toString() << "setup complete";
        info->finish(Thing::ThingErrorNoError);
    });
}

void IntegrationPluginTelegram::executeAction(ThingActionInfo *info)
{
    Thing *thing = info->thing();
    Action action = info->action();

    qCDebug(dcTelegram()) << "Executing action" << action.actionTypeId() << "for" << thing->name() << thing->id().toString();

    QString token = info->thing()->paramValue(telegramThingTokenParamTypeId).toString();
    int chatId = info->thing()->paramValue(telegramThingChatIdParamTypeId).toInt();

    QString title = action.paramValue(telegramNotifyActionTitleParamTypeId).toString();
    QString body = action.paramValue(telegramNotifyActionBodyParamTypeId).toString();
    QString message = title + "\n" + body;

    QString url = QString("https://api.telegram.org/bot%1/sendMessage?chat_id=%2&text=%3").arg(token).arg(chatId).arg(message);

    QNetworkRequest request(url);

    QNetworkReply *reply = hardwareManager()->networkManager()->get(request);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    connect(reply, &QNetworkReply::finished, info, [reply, info]{
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(dcTelegram()) << "Sending message failed for" << info->thing()->name() << info->thing()->id() << reply->errorString() << reply->error();
            emit info->finish(Thing::ThingErrorHardwareNotAvailable);
            return;
        }

        QByteArray data = reply->readAll();

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(dcTelegram()) << "Error reading reply from Telegram for" << info->thing()->name() << info->thing()->id().toString() << error.errorString();
            qCWarning(dcTelegram()) << qUtf8Printable(data);
            info->finish(Thing::ThingErrorHardwareFailure);
            return;
        }

        QVariantMap replyMap = jsonDoc.toVariant().toMap();
        if (!replyMap.value("ok").toBool()) {
            qCWarning(dcTelegram()) << "Error sending message." << info->thing()->name() << info->thing()->id().toString();
            info->finish(Thing::ThingErrorAuthenticationFailure, QT_TR_NOOP("The Telegram bot account seems to be disabled."));
            return;
        }

        qCDebug(dcTelegram()) << "Message sent successfully";
        info->finish(Thing::ThingErrorNoError);
    });
}

