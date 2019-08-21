#ifndef RECEIVEKEYEVENT_H
#define RECEIVEKEYEVENT_H

#include <QObject>

class ReceiveKeyEvent : public QObject
{
    Q_OBJECT
public:
    explicit ReceiveKeyEvent(QObject *parent = nullptr);
    bool init();

private:
    int m_fd;

signals:
    void keyEventReceived(Qt::Key key, bool pressed);
    void longPressed(Qt::Key key);
    void doublePressed(Qt::Key key);
};

#endif // RECEIVEKEYEVENT_H
