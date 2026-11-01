#include "update_downloader.h"

#include <QDir>
#include <QIODevice>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QUrl>
#include <libcockatrice/network/network_manager.h>

namespace
{
constexpr int DOWNLOAD_TIMEOUT_MS = 30000;
constexpr qint64 DOWNLOAD_BUFFER_SIZE = 256 * 1024;
constexpr qint64 WRITE_CHUNK_SIZE = 64 * 1024;

QString installerFileName(const QUrl &url)
{
    QString name = QUrl::fromPercentEncoding(url.fileName(QUrl::FullyEncoded).toUtf8());
    // The URL supplies a filename, not a path. Encoded separators must not escape
    // the private directory, and the installer suffix must survive URL queries.
    for (QChar &character : name) {
        if (character.unicode() < 32 || QStringLiteral("<>:\"/\\|?*").contains(character)) {
            character = QLatin1Char('_');
        }
    }
    while (name.endsWith(QLatin1Char('.')) || name.endsWith(QLatin1Char(' '))) {
        name.chop(1);
    }
    return name.isEmpty() ? QStringLiteral("update") : name;
}
} // namespace

UpdateDownloader::UpdateDownloader(QObject *parent) : QObject(parent)
{
    netMan = NetworkManagerFactory::create(this);
    connect(this, &UpdateDownloader::stopDownload, this, &UpdateDownloader::discardDownload);
}

UpdateDownloader::~UpdateDownloader()
{
    discardDownload();
}

void UpdateDownloader::beginDownload(QUrl downloadUrl)
{
    // Retire the old reply before abort(), which may emit its signals synchronously.
    discardDownload();

    downloadDir = std::make_unique<QTemporaryDir>(QDir::temp().filePath(QStringLiteral("cockatrice-update-XXXXXX")));
    if (!downloadDir->isValid()) {
        failDownload(tr("Could not create a temporary directory for the download."));
        return;
    }
    fileName = downloadDir->filePath(installerFileName(downloadUrl));
    output = std::make_unique<QSaveFile>(fileName);
    output->setDirectWriteFallback(false);
    if (!output->open(QIODevice::WriteOnly)) {
        failDownload(tr("Could not open the file for writing: %1").arg(output->errorString()));
        return;
    }

    QNetworkRequest request(downloadUrl);
    request.setTransferTimeout(DOWNLOAD_TIMEOUT_MS);
    response = netMan->get(request);
    response->setReadBufferSize(DOWNLOAD_BUFFER_SIZE);
    connect(response, &QNetworkReply::readyRead, this, [this] {
        if (sender() == response) {
            writeAvailableData();
        }
    });
    connect(response, &QNetworkReply::finished, this, &UpdateDownloader::fileFinished);
    connect(response, &QNetworkReply::errorOccurred, this, &UpdateDownloader::downloadError);
    connect(response, &QNetworkReply::downloadProgress, this, &UpdateDownloader::downloadProgress);
}

void UpdateDownloader::discardDownload()
{
    const auto oldReply = response;
    response.clear();
    if (oldReply) {
        disconnect(oldReply, nullptr, this, nullptr);
        if (!oldReply->isFinished() && oldReply->error() == QNetworkReply::NoError) {
            oldReply->abort();
        }
        if (oldReply) {
            oldReply->deleteLater();
        }
    }
    if (output) {
        output->cancelWriting();
        output.reset();
    }
    downloadDir.reset();
    fileName.clear();
    bytesWritten = 0;
}

void UpdateDownloader::failDownload(const QString &message)
{
    // Clear operation state before notifying callers: an error receiver may retry
    // or destroy the downloader while handling the signal.
    discardDownload();
    emit error(message);
}

bool UpdateDownloader::writeAvailableData()
{
    while (response && response->bytesAvailable() > 0) {
        const QByteArray data = response->read(WRITE_CHUNK_SIZE);
        if (data.isEmpty() || !output || output->write(data) != data.size()) {
            failDownload(tr("Could not write the downloaded file to disk."));
            return false;
        }
        bytesWritten += data.size();
    }
    return true;
}

void UpdateDownloader::downloadError(QNetworkReply::NetworkError networkError)
{
    if (!response || sender() != response) {
        return;
    }
    if (networkError == QNetworkReply::OperationCanceledError) {
        discardDownload();
        return;
    }
    failDownload(response->errorString());
}

void UpdateDownloader::fileFinished()
{
    if (!response || sender() != response) {
        return;
    }
    if (response->error() != QNetworkReply::NoError) {
        downloadError(response->error());
        return;
    }
    if (!writeAvailableData()) {
        return;
    }
    if (bytesWritten == 0) {
        failDownload(tr("The downloaded update is empty."));
        return;
    }
    // Check buffered writes before committing. In particular, a failed flush
    // must never turn a partial installer into a successful download.
    if (!output->flush() || !output->commit()) {
        failDownload(tr("Could not save the downloaded file: %1").arg(output->errorString()));
        return;
    }

    const QUrl downloadedFile = QUrl::fromLocalFile(fileName);
    output.reset();
    // A successfully launched installer may outlive both this object and the
    // application. Only successful downloads retain their private directory.
    downloadDir->setAutoRemove(false);
    discardDownload();
    emit downloadSuccessful(downloadedFile);
}

void UpdateDownloader::downloadProgress(qint64 bytesRead, qint64 totalBytes)
{
    if (response && sender() == response) {
        emit progressMade(bytesRead, totalBytes);
    }
}
