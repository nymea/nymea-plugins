/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  nymea is free software: you can redistribute it and/or modify          *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  nymea is distributed in the hope that it will be useful,               *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with nymea. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "emitkeyevent.h"
#include "keymap.h"
#include "extern-plugininfo.h"

#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

EmitKeyEvent::EmitKeyEvent(QObject *parent) : QObject(parent)
{

}

EmitKeyEvent::~EmitKeyEvent()
{
    ioctl(m_uinputFd, UI_DEV_DESTROY);
    close(m_uinputFd);
    m_uinputFd = -1;
}

bool EmitKeyEvent::init()
{
    m_uinputFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_uinputFd == -1) {
        qWarning(dcKeyEvent) << "Failed to open '/dev/uinput'";
        return false;
    }

    /*foreach (Qt::Key key, Qt->idKeyMap.keys().toSet()) {
        if (ioctl(m_uinputFd, UI_SET_EVBIT, EV_KEY) < 0) {
            qWarning(dcKeyEvent) << "Failed to enable keys for the uinput device";
            return false;
        }

        if (ioctl(m_uinputFd, UI_SET_KEYBIT, d->qtKeyToUiKey(key)) < 0) {
            qWarning(dcKeyEvent) << "Failed to enable Qt key" << key;
            return false;
        }
    }*/

    uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "nymea");
    uidev.id.bustype = BUS_VIRTUAL;
    uidev.id.vendor  = 0x00;
    uidev.id.product = 0x00;
    uidev.id.version = 0x01;
    if (write(m_uinputFd, &uidev, sizeof(uidev)) != sizeof(uidev)) {
        qWarning(dcKeyEvent) << "Failed to write device information to uinput";
        return false;
    }

    if (ioctl( m_uinputFd, UI_DEV_CREATE) < 0) {
        qWarning(dcKeyEvent) << "Failed to create uinput device";
        return false;
    }

    return true;
}

bool EmitKeyEvent::sendKeyEvent(Qt::Key key, bool pressed)
{

    if (m_uinputFd == -1) {
        qWarning(dcKeyEvent()) << "Cannot send key event, device has not been created";
        return false;
    }

    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_KEY;
    ev.code = key;
    ev.value = pressed;
    if (write(m_uinputFd, &ev, sizeof(ev)) != sizeof(ev)) {
        qWarning(dcKeyEvent()) << "Cannot write input event to uintput";
        return false;
    }
    qDebug(dcKeyEvent()) << "key event sent to uinput device succesfully";
    return true;
}
