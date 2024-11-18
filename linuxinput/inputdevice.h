#ifndef INPUTDEVICE_H
#define INPUTDEVICE_H

#include <QDir>
#include <QDebug>
#include <QObject>
#include <QThread>

#include <linux/input-event-codes.h>
#include <linux/input.h>

class InputDevice : public QObject
{
    Q_OBJECT
public:
    enum Type {
        Unknown,
        Keyboard,
        Mouse,
        Other
    };
    Q_ENUM(Type)

    typedef struct Info {
        int id = -1;
        Type type = Unknown;
        QString name;
        QString description;
        QString phys;
        QString eventFilePath;
    } InputDeviceInfo;

    explicit InputDevice(QObject *parent = nullptr);
    explicit InputDevice(Info inputDeviceInfo, QObject *parent = nullptr);
    ~InputDevice();

    InputDevice::Info inputDeviceInfo() const;

    static QList<InputDevice::Info> availableInputDevices();

signals:
    void keyPressed(quint16 code, qint32 value);

private slots:
    void onEventOccurred(quint16 type, quint16 code, qint32 value);

private:
    QString m_eventFileName;
    static QString readFileContent(const QString &fileName);
    Info m_inputDeviceInfo;
    QThread *m_pollThread = nullptr;

    static InputDevice::Info parseDeviceInfo(Type type, const QFileInfo &inputLinkFileInfo);
};

QDebug operator<<(QDebug debug, InputDevice::Info inputDeviceInfo);
QDebug operator<<(QDebug debug, InputDevice *inputDevice);


#endif // INPUTDEVICE_H
