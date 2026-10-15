// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026, chargebyte austria GmbH

#ifndef RFIDPROVIDERCLIENT_H
#define RFIDPROVIDERCLIENT_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QTimer>
#include <QVariantMap>
#include <functional>

class RfidProviderClient : public QObject
{
    Q_OBJECT
public:
    explicit RfidProviderClient(QObject *parent = nullptr,
                                const QDBusConnection &connection = QDBusConnection::systemBus());
    bool available() const;
    bool ready() const;
    void probe(std::function<void(bool)> callback = {});

signals:
    void availabilityChanged();
    void tagDetected(const QString &code, const QVariantMap &metadata);

private slots:
    void onTagDetected(const QString &code, const QVariantMap &metadata, const QDBusMessage &message);
    void onPropertiesChanged(const QString &interface, const QVariantMap &changed,
                             const QStringList &invalidated, const QDBusMessage &message);

private:
    QDBusConnection m_connection;
    QTimer m_probeTimer;
    QList<std::function<void(bool)>> m_callbacks;
    QString m_owner;
    quint64 m_generation = 0;
    bool m_probing = false;
    bool m_available = false;
    bool m_ready = false;

    void finishProbe(bool available, bool ready = false);
};

#endif // RFIDPROVIDERCLIENT_H
