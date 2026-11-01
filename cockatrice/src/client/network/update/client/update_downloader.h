/**
 * @file update_downloader.h
 * @ingroup ClientUpdate
 */
//! \todo Document this file.

#ifndef COCKATRICE_UPDATEDOWNLOADER_H
#define COCKATRICE_UPDATEDOWNLOADER_H

#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <memory>
#include <qtmetamacros.h>

class QNetworkAccessManager;
class QSaveFile;
class QTemporaryDir;

class UpdateDownloader : public QObject
{
    Q_OBJECT
public:
    explicit UpdateDownloader(QObject *parent);
    ~UpdateDownloader() override;
    void beginDownload(QUrl url);
signals:
    void downloadSuccessful(QUrl filepath);
    void progressMade(qint64 bytesRead, qint64 totalBytes);
    void error(QString errorString);
    void stopDownload();

private:
    QNetworkAccessManager *netMan;
    QPointer<QNetworkReply> response;
    std::unique_ptr<QTemporaryDir> downloadDir;
    std::unique_ptr<QSaveFile> output;
    QString fileName;
    qint64 bytesWritten = 0;

    void discardDownload();
    void failDownload(const QString &message);
    bool writeAvailableData();
private slots:
    void fileFinished();
    void downloadProgress(qint64 bytesRead, qint64 totalBytes);
    void downloadError(QNetworkReply::NetworkError);
};

#endif // COCKATRICE_UPDATEDOWNLOADER_H
