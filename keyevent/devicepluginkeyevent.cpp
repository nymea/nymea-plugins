#include "devicepluginkeyevent.h"
#include "devices/device.h"
#include "plugininfo.h"


DevicePluginKeyEvent::DevicePluginKeyEvent()
{
}

Device::DeviceSetupStatus DevicePluginKeyEvent::setupDevice(Device *device)
{
    if(!m_keyEvent) {
        m_keyEvent = new KeyEvent(this);
        if (!m_keyEvent->create("Test")) {
            return Device::DeviceSetupStatusFailure;
        }
    }

    if (device->deviceClassId() == keyEventDeviceClassId) {
        return Device::DeviceSetupStatusSuccess;
    }
    return Device::DeviceSetupStatusFailure;
}

void DevicePluginKeyEvent::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == keyEventDeviceClassId) {

    }
}

Device::DeviceError DevicePluginKeyEvent::executeAction(Device *device, const Action &action)
{
    if (device->deviceClassId() == keyEventDeviceClassId) {

        if (action.actionTypeId() == keyEventPlayActionTypeId) {
            QKeyEvent *event = new QKeyEvent (QEvent::KeyPress, Qt::Key_Play, Qt::NoModifier);


            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == keyEventPauseActionTypeId) {
            QKeyEvent *event = new QKeyEvent (QEvent::KeyPress, Qt::Key_Pause, Qt::NoModifier);


            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == keyEventStopActionTypeId) {
            QKeyEvent *event = new QKeyEvent (QEvent::KeyPress, Qt::Key_Stop, Qt::NoModifier);


            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == keyEventVolumeDownActionTypeId) {
            QKeyEvent *event = new QKeyEvent (QEvent::KeyPress, Qt::Key_VolumeDown, Qt::NoModifier);


            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == keyEventVolumeUpActionTypeId) {
            QKeyEvent *event = new QKeyEvent (QEvent::KeyPress, Qt::Key_VolumeUp, Qt::NoModifier);


            return Device::DeviceErrorNoError;
        }
        if (action.actionTypeId() == keyEventMuteActionTypeId) {
            QKeyEvent *event = new QKeyEvent (QEvent::KeyPress, Qt::Key_VolumeMute, Qt::NoModifier);


            return Device::DeviceErrorNoError;
        }

        return Device::DeviceErrorActionTypeNotFound;
    }
    return Device::DeviceErrorDeviceClassNotFound;
}
