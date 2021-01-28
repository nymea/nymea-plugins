#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <QObject>
#include <QHash>

class Discovery : public QObject
{
    Q_OBJECT
public:
    struct Result {
        QString name;
        QString macAddress;
        QString ipAddress;
        int port;
    };
    explicit Discovery(QObject *parent = nullptr);

    void discover();

    QHash<QString, Result> results() const;

signals:
    void finished(bool error);

private:
    QHash<QString, Result> m_results;
};

#endif // DISCOVERY_H
