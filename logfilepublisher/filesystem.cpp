#include "filesystem.h"

FileSystem::FileSystem(QObject *parent) : QObject(parent)
{

}

Device::BrowseResult FileSystem::browse(const QString &itemId, Device::BrowseResult &result)
{

}

Device::BrowserItemResult FileSystem::browserItem(const QString &itemId, Device::BrowserItemResult &result)
{
    qCDebug(dcLogfileBrowser) << "Getting details for" << itemId;
    QString idString = itemId;
    QString method;
    QVariantMap params;
    if (idString.startsWith("song:")) {
        idString.remove(QRegExp("^song:"));
        params.insert("songid", idString.toInt());
        method = "AudioLibrary.GetSongDetails";
    } else if (idString.startsWith("movie:")) {
        idString.remove(QRegExp("^movie:"));
        params.insert("movieid", idString.toInt());
        method = "VideoLibrary.GetMovieDetails";
    } else if (idString.startsWith("episode:")) {
        idString.remove(QRegExp("^episode:"));
        params.insert("episodeid", idString.toInt());
        method = "VideoLibrary.GetEpisodeDetails";
    } else if (idString.startsWith("musicvideo:")) {
        idString.remove(QRegExp("^musicvideo:"));
        params.insert("musicvideoid", idString.toInt());
        method = "VideoLibrary.GetMusicVideoDetails";
    } else {
        qCWarning(dcKodi()) << "Unhandled browserItem request!" << itemId;
        result.status = Device::DeviceErrorUnsupportedFeature;
        return result;
    }
    int id = m_jsonHandler->sendData(method, params);
    m_pendingBrowserItemRequests.insert(id, result);
    result.status = Device::DeviceErrorAsync;
    return result;
}

Device::DeviceError FileSystem::launchBrowserItem(const QString &itemId)
{

}

int FileSystem::executeBrowserItemAction(const QString &itemId, const ActionTypeId &actionTypeId)
{

}
