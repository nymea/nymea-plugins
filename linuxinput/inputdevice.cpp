#include "inputdevice.h"

#include <QDir>
#include <QDebug>
#include <QFile>

#include "extern-plugininfo.h"
#include "inputdeviceeventmonitor.h"

#include <libevdev/libevdev.h>

#include <unistd.h>
#include <fcntl.h>

InputDevice::InputDevice(QObject *parent)
    : QObject{parent}
{

}

InputDevice::InputDevice(Info inputDeviceInfo, QObject *parent)
    : QObject{parent},
    m_inputDeviceInfo{inputDeviceInfo},
    m_pollThread{new QThread(nullptr)}
{
    qCDebug(dcLinuxInput()) << "Constructing" << inputDeviceInfo;
    InputDeviceEventMonitor *monitor = new InputDeviceEventMonitor(inputDeviceInfo.eventFilePath, nullptr);
    monitor->moveToThread(m_pollThread);

    connect(m_pollThread, &QThread::started, monitor, &InputDeviceEventMonitor::process);
    connect(monitor, &InputDeviceEventMonitor::finished, m_pollThread, &QThread::quit);
    connect(monitor, &InputDeviceEventMonitor::finished, monitor, &InputDeviceEventMonitor::deleteLater);
    connect(m_pollThread, &QThread::finished, monitor, &InputDeviceEventMonitor::deleteLater);
    connect(m_pollThread, &QThread::finished, m_pollThread, [this](){
        qCDebug(dcLinuxInput()) << "Monitoring thread finished" << m_inputDeviceInfo;
        m_pollThread->deleteLater();
        m_pollThread = nullptr;
    });

    connect(monitor, &InputDeviceEventMonitor::eventOccurred, this, &InputDevice::onEventOccurred);

    qCDebug(dcLinuxInput()) << "Start monitoring" << inputDeviceInfo;
    m_pollThread->start();
}

InputDevice::~InputDevice()
{
    if (m_pollThread) {
        m_pollThread->terminate();
    }
}

InputDevice::Info InputDevice::inputDeviceInfo() const
{
    return m_inputDeviceInfo;
}

QList<InputDevice::Info> InputDevice::availableInputDevices()
{
    QList<Info> inputeDevices;
    QStringList addedInputIds;
    QDir inputByIdDir("/dev/input/by-path/");

    // Get the keyboards
    qCDebug(dcLinuxInput()) << "---------- Keyboards:";
    foreach (const QString &inputIdName, inputByIdDir.entryList({"*event-kbd"}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        Info info = parseDeviceInfo(Keyboard, QFileInfo(inputByIdDir.absolutePath() + QDir::separator() + inputIdName));
        qCDebug(dcLinuxInput()) << " --> " << info;
        inputeDevices.append(info);
        addedInputIds.append(inputIdName);
    }

    qCDebug(dcLinuxInput()) << "---------- Mouse:";
    foreach (const QString &inputIdName, inputByIdDir.entryList({"*event-mouse"}, QDir::Dirs | QDir::NoDotAndDotDot)) {
        Info info = parseDeviceInfo(Mouse, QFileInfo(inputByIdDir.absolutePath() + QDir::separator() + inputIdName));
        qCDebug(dcLinuxInput()) << " --> " << info;
        inputeDevices.append(info);
        addedInputIds.append(inputIdName);
    }

    qCDebug(dcLinuxInput()) << "---------- Other:";
    foreach (const QString &inputIdName, inputByIdDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (addedInputIds.contains(inputIdName))
            continue;

        Info info = parseDeviceInfo(Other, QFileInfo(inputByIdDir.absolutePath() + QDir::separator() + inputIdName));
        qCDebug(dcLinuxInput()) << " --> " << info;
        inputeDevices.append(info);
        addedInputIds.append(inputIdName);
    }

    return inputeDevices;
}

void InputDevice::onEventOccurred(quint16 type, quint16 code, qint32 value)
{
    if (type != EV_KEY)
        return;

    qCDebug(dcLinuxInput()) << "Event occurred" << type << code << value;
    emit keyPressed(code, value);
}

QString InputDevice::readFileContent(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(dcLinuxInput()) << "Could not open file" << fileName << ":"  << file.errorString();
        return QString();
    }

    QString content;
    QTextStream in(&file);
    content = in.readAll();
    file.close();

    return content.trimmed();
}

InputDevice::Info InputDevice::parseDeviceInfo(Type type, const QFileInfo &inputLinkFileInfo)
{
    QFileInfo inputFileInfo(inputLinkFileInfo.symLinkTarget());

    QDir inputsDir("/sys/class/input/");
    QDir inputDir(inputsDir.absolutePath() + QDir::separator() + inputFileInfo.baseName() + QDir::separator() + "device");

    Info info;
    info.type = type;
    info.id = QString(inputFileInfo.baseName()).remove("event").toInt();
    info.name = readFileContent(inputDir.absolutePath() + QDir::separator() + "name");
    info.phys = readFileContent(inputDir.absolutePath() + QDir::separator() + "phys");
    info.eventFilePath = inputLinkFileInfo.symLinkTarget();

    return info;
}

QDebug operator<<(QDebug debug, InputDevice::Info inputDeviceInfo)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Info(" << inputDeviceInfo.type
                    << ", name: " << inputDeviceInfo.name
                    << ", Phys: " << inputDeviceInfo.phys
                    << ", " << inputDeviceInfo.eventFilePath
                    << ')';
    return debug;
}

QDebug operator<<(QDebug debug, InputDevice *inputDevice)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "InputDevice(" << inputDevice->inputDeviceInfo() << ')';
    return debug;
}


