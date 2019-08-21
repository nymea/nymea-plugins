#include "receivekeyevent.h"
#include "extern-plugininfo.h"

#include <errno.h>
#include <linux/kd.h>
#include <linux/input.h>

#include <QSocketNotifier>

ReceiveKeyEvent::ReceiveKeyEvent(QObject *parent) : QObject(parent)
{

}

bool ReceiveKeyEvent::init()
{
    QString dev = QLatin1String("/dev/input/event1");
    m_fd = QT_OPEN(dev.toLocal8Bit().constData(), O_RDWR, 0);
    if (m_fd == -1) {
        qWarning(dcKeyEvent) << "Cannot open input device" << dev << strerror(errno);
        return false;
    }

    ioctl(m_fd, EVIOCSREP, kbdrep);

    QSocketNotifier *notifier;
    notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), this, SLOT(readKeycode()));
}

