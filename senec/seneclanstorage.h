#ifndef SENECLANSTORAGE_H
#define SENECLANSTORAGE_H

#include <QObject>

class SenecLanStorage : public QObject
{
    Q_OBJECT
public:
    explicit SenecLanStorage(QObject *parent = nullptr);

signals:
};

#endif // SENECLANSTORAGE_H
