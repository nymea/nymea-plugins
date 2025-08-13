#ifndef ARDUINOFLASHER_H
#define ARDUINOFLASHER_H

#include <QObject>
#include <QFileInfo>
#include <QProcess>

class ArduinoFlasher : public QObject
{
    Q_OBJECT
public:
    enum Arduino {
        ArduinoUno,
        ArduinoMiniPro5V,
        ArduinoMiniPro3V,
        ArduinoNano
    };
    Q_ENUM(Arduino)

    explicit ArduinoFlasher(Arduino model, const QString &serialPort, QObject *parent = nullptr);

    bool available() const;

    QString availableVersion() const;

    bool flashFirmware();

signals:
    void flashProcessFinished(bool success);

private:
    Arduino m_model = ArduinoUno;
    QString m_serialPort;

    QProcess *m_flashProcess = nullptr;
    bool m_available = false;
    QString m_availableVersion;
    QString m_firmwareFileName;

    QStringList m_flashProcessArguments;

    QVariantMap loadReleaseInfo(const QString &firmwareDirectoryPath);

private slots:
    void onProcessFinished(int returnCode, QProcess::ExitStatus exitStatus);

};

#endif // ARDUINOFLASHER_H
