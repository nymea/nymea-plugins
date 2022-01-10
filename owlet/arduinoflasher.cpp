#include "arduinoflasher.h"
#include "extern-plugininfo.h"

#include <QDir>
#include <QJsonDocument>

ArduinoFlasher::ArduinoFlasher(Arduino model, const QString &serialPort, QObject *parent) :
    QObject(parent),
    m_model(model),
    m_serialPort(serialPort)
{
    QString firmwareBasePath = "usr/share/nymea/owlet/firmware";

    switch (model) {
    case ArduinoUno: {
        QString firmwarePath = firmwareBasePath + QDir::separator() + "arduino-uno";
        QVariantMap releaseInfo = loadReleaseInfo(firmwarePath);
        if (releaseInfo.isEmpty())
            return;

        m_availableVersion = releaseInfo.value("version").toString();
        m_firmwareFileName = firmwarePath + QDir::separator() + releaseInfo.value("firmwareFile").toString();

        if (!QFileInfo::exists(m_firmwareFileName)) {
            qCWarning(dcOwlet()) << "ArduinoFlasher: Could not find the firmware file for flashing" << m_firmwareFileName;
            return;
        }

        m_flashProcessArguments << "-c" << "avrisp" << "-p" << "m328p" << "-P" << serialPort << "-b" << "19200" << "-U" << QString("flash:w:%1:i").arg(m_firmwareFileName);
        m_available = true;
        break;
    }
    case ArduinoNano: {
        break;
    }
    case ArduinoMiniPro3V: {
        break;
    }
    case ArduinoMiniPro5V: {
        break;
    }
    }
}

QString ArduinoFlasher::availableVersion() const
{
    return m_availableVersion;
}

bool ArduinoFlasher::flashFirmware()
{
    if (!m_available)
        return false;

    if (m_flashProcess)
        return false;

    m_flashProcess = new QProcess(this);
    m_flashProcess->setProgram("avrdude");
    m_flashProcess->setArguments(m_flashProcessArguments);
    connect(m_flashProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onProcessFinished(int,QProcess::ExitStatus)));
    m_flashProcess->start();

    return true;
}

QVariantMap ArduinoFlasher::loadReleaseInfo(const QString &firmwareDirectoryPath)
{
    QFileInfo releaseFileInfo(firmwareDirectoryPath + QDir::separator() + "release.json");
    if (!releaseFileInfo.exists()) {
        qCWarning(dcOwlet()) << "ArduinoFlasher: Could not load release info. The release file does not exist" << releaseFileInfo.absoluteFilePath();
        return QVariantMap();
    }

    QFile releaseFile;
    releaseFile.setFileName(releaseFileInfo.absoluteFilePath());
    if (!releaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(dcOwlet()) << "ArduinoFlasher: Could not open release file" << releaseFileInfo.absoluteFilePath();
        return QVariantMap();
    }

    QByteArray releaseJsonData = releaseFile.readAll();
    releaseFile.close();

    return QJsonDocument::fromJson(releaseJsonData).toVariant().toMap();
}

void ArduinoFlasher::onProcessFinished(int returnCode, QProcess::ExitStatus exitStatus)
{
    if (returnCode != EXIT_SUCCESS || exitStatus != QProcess::NormalExit) {
        qCWarning(dcOwlet()) << "ArduinoFlasher: Flash process finished with error" << returnCode << exitStatus;
        emit flashProcessFinished(false);
    } else {
        qCDebug(dcOwlet()) << "ArduinoFlasher: Flash process finished successfully";
        emit flashProcessFinished(true);
    }

    m_flashProcess->deleteLater();
    m_flashProcess = nullptr;
}
