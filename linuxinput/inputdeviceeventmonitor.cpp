#include "inputdeviceeventmonitor.h"
#include "extern-plugininfo.h"

#include <QFile>
#include <QDebug>

#include <fcntl.h>
#include <unistd.h>
#include <libevdev/libevdev.h>

InputDeviceEventMonitor::InputDeviceEventMonitor(const QString &fileName, QObject *parent)
    : QObject{parent},
    m_fileName{fileName}
{

}

void InputDeviceEventMonitor::process()
{
    struct libevdev *device = nullptr;

    int fd = open(m_fileName.toStdString().c_str(), O_RDWR|O_CLOEXEC);
    if (fd < 0) {
        qCWarning(dcLinuxInput()) << "Could not open file" << m_fileName;
        emit finished();
        return;
    }

    int status = libevdev_new_from_fd(fd, &device);
    if (status != 0) {
        qCWarning(dcLinuxInput()) << "Failed to create evdev from file descriptor";
        close(fd);
        emit finished();
        return;
    }

    qCDebug(dcLinuxInput()) << "Start monitoring events on" << m_fileName;
    qCDebug(dcLinuxInput()) << "- name: " << libevdev_get_name(device);
    qCDebug(dcLinuxInput()) << "- phys: " << libevdev_get_phys(device);

    struct input_event event = {};
    auto hasError = [](int v) {
        return v < 0 && v != -EAGAIN;
    };
    auto hasNextEvent = [](int v) {
        return v >= 0;
    };

    while (status = libevdev_next_event(device, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &event), !hasError(status)) {
        if (!hasNextEvent(status))
            continue;

        emit eventOccurred(event.type, event.code, event.value);
    }

    libevdev_free(device);
    device = nullptr;

    close(fd);

    emit finished();
}
