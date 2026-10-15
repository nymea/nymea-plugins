// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026, chargebyte austria GmbH

#include "rfidproviderclient.h"

#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QXmlStreamReader>

namespace {
const QString service = QStringLiteral("io.nymea.RfidProvider");
const QString path = QStringLiteral("/io/nymea/RfidProvider");
const QString interface = QStringLiteral("io.nymea.RfidProvider1");
const QString properties = QStringLiteral("org.freedesktop.DBus.Properties");

bool compatible(const QString &xml)
{
    QXmlStreamReader reader(xml);
    bool foundSignal = false;
    QMap<QString, QString> propertyTypes;
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String("node"))
            continue;
        if (reader.name() != QLatin1String("interface") || reader.attributes().value("name") != interface) {
            reader.skipCurrentElement();
            continue;
        }
        while (reader.readNextStartElement()) {
            const auto attributes = reader.attributes();
            if (reader.name() == QLatin1String("property")) {
                const QString access = attributes.value("access").toString();
                if (access == QLatin1String("read") || access == QLatin1String("readwrite"))
                    propertyTypes.insert(attributes.value("name").toString(), attributes.value("type").toString());
                reader.skipCurrentElement();
            } else if (reader.name() == QLatin1String("signal") && attributes.value("name") == QLatin1String("tagDetected")) {
                QString signature;
                while (reader.readNextStartElement()) {
                    if (reader.name() == QLatin1String("arg"))
                        signature += reader.attributes().value("type").toString();
                    reader.skipCurrentElement();
                }
                foundSignal = signature == QLatin1String("sa{sv}");
            } else {
                reader.skipCurrentElement();
            }
        }
    }
    return !reader.hasError() && foundSignal && propertyTypes.value("Backend") == QLatin1String("s")
            && propertyTypes.value("Ready") == QLatin1String("b")
            && propertyTypes.value("SupportsPresence") == QLatin1String("b")
            && propertyTypes.value("LastError") == QLatin1String("s");
}
}

RfidProviderClient::RfidProviderClient(QObject *parent, const QDBusConnection &connection) :
    QObject(parent), m_connection(connection)
{
    m_probeTimer.setSingleShot(true);
    m_probeTimer.setInterval(1500);
    connect(&m_probeTimer, &QTimer::timeout, this, [this]() { finishProbe(false); });
    auto watcher = new QDBusServiceWatcher(service, m_connection, QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
            [this](const QString &, const QString &, const QString &newOwner) {
        ++m_generation;
        m_owner.clear();
        m_available = false;
        m_ready = false;
        emit availabilityChanged();
        if (newOwner.isEmpty()) {
            finishProbe(false);
        } else {
            m_probing = false;
            probe();
        }
    });
    m_connection.connect(service, path, interface, "tagDetected", this,
                         SLOT(onTagDetected(QString,QVariantMap,QDBusMessage)));
    m_connection.connect(service, path, properties, "PropertiesChanged", this,
                         SLOT(onPropertiesChanged(QString,QVariantMap,QStringList,QDBusMessage)));
}

bool RfidProviderClient::available() const { return m_available; }
bool RfidProviderClient::ready() const { return m_available && m_ready; }

void RfidProviderClient::probe(std::function<void(bool)> callback)
{
    if (callback)
        m_callbacks.append(std::move(callback));
    if (m_probing)
        return;
    m_probing = true;
    const quint64 generation = ++m_generation;
    m_probeTimer.start();
    auto request = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                                 "org.freedesktop.DBus", "GetNameOwner");
    request << service;
    auto ownerCall = new QDBusPendingCallWatcher(m_connection.asyncCall(request, 1500), this);
    connect(ownerCall, &QDBusPendingCallWatcher::finished, this, [this, ownerCall, generation]() {
        ownerCall->deleteLater();
        if (!m_probing || generation != m_generation)
            return;
        QDBusPendingReply<QString> ownerReply = *ownerCall;
        if (ownerReply.isError()) {
            finishProbe(false);
            return;
        }
        m_owner = ownerReply.value();
        auto request = QDBusMessage::createMethodCall(m_owner, path, "org.freedesktop.DBus.Introspectable", "Introspect");
        auto introspectCall = new QDBusPendingCallWatcher(m_connection.asyncCall(request, 1500), this);
        connect(introspectCall, &QDBusPendingCallWatcher::finished, this, [this, introspectCall, generation]() {
            introspectCall->deleteLater();
            if (!m_probing || generation != m_generation)
                return;
            QDBusPendingReply<QString> reply = *introspectCall;
            if (reply.isError() || !compatible(reply.value())) {
                finishProbe(false);
                return;
            }
            auto request = QDBusMessage::createMethodCall(m_owner, path, properties, "GetAll");
            request << interface;
            auto propertiesCall = new QDBusPendingCallWatcher(m_connection.asyncCall(request, 1500), this);
            connect(propertiesCall, &QDBusPendingCallWatcher::finished, this, [this, propertiesCall, generation]() {
                propertiesCall->deleteLater();
                if (!m_probing || generation != m_generation)
                    return;
                QDBusPendingReply<QVariantMap> reply = *propertiesCall;
                const QVariantMap values = reply.value();
                const bool valid = !reply.isError() && values.value("Ready").userType() == QMetaType::Bool
                        && values.value("Backend").userType() == QMetaType::QString
                        && values.value("SupportsPresence").userType() == QMetaType::Bool
                        && values.value("LastError").userType() == QMetaType::QString;
                finishProbe(valid, values.value("Ready").toBool());
            });
        });
    });
}

void RfidProviderClient::finishProbe(bool available, bool ready)
{
    m_probeTimer.stop();
    m_probing = false;
    ++m_generation;
    const bool changed = m_available != available || m_ready != ready;
    m_available = available;
    m_ready = ready;
    if (!available)
        m_owner.clear();
    if (changed)
        emit availabilityChanged();
    const auto callbacks = std::move(m_callbacks);
    m_callbacks.clear();
    for (const auto &callback : callbacks)
        callback(available);
}

void RfidProviderClient::onTagDetected(const QString &code, const QVariantMap &metadata, const QDBusMessage &message)
{
    if (ready() && message.service() == m_owner && !code.isEmpty())
        emit tagDetected(code, metadata);
}

void RfidProviderClient::onPropertiesChanged(const QString &changedInterface, const QVariantMap &changed,
                                            const QStringList &invalidated, const QDBusMessage &message)
{
    if (!m_available || changedInterface != interface || message.service() != m_owner)
        return;
    if (invalidated.contains("Ready")) {
        m_ready = false;
        emit availabilityChanged();
        probe();
    } else if (changed.contains("Ready")) {
        m_ready = changed.value("Ready").userType() == QMetaType::Bool && changed.value("Ready").toBool();
        emit availabilityChanged();
    }
}
