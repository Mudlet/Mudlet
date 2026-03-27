#ifndef DBLSQD_FEED_H
#define DBLSQD_FEED_H

#include "Release.h"

#include <QCoreApplication>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class QNetworkReply;
class QTemporaryFile;

namespace dblsqd {

class Feed : public QObject
{
    Q_OBJECT

public:
    Feed();

    void setRepo(const QString& owner, const QString& repo, bool prerelease = false, const QString& os = QString(), const QString& arch = QString());
    QUrl getUrl();

    void load();
    void downloadRelease(Release release);

    QList<Release> getUpdates(Release currentRelease = Release(QCoreApplication::applicationVersion()));
    QList<Release> getReleases();
    QTemporaryFile* getDownloadFile();
    bool isReady();

signals:
    void ready();
    void loadError(QString message);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    void downloadError(QString message);

private:
    QUrl url;

    QList<Release> releases;

    void makeDownloadRequest(QUrl url);
    void fetchChecksums(const QUrl& checksumsUrl);

    QNetworkAccessManager nam;
    QNetworkReply* feedReply;
    Release release;
    QNetworkReply* downloadReply;
    QTemporaryFile* downloadFile;
    bool _ready;

    QString m_owner;
    QString m_repo;
    bool m_prerelease{false};
    QString m_os;
    QString m_arch;

    static QString detectOs();
    static QString detectArch();

private slots:
    void handleFeedFinished();
    void handleDownloadProgress(qint64, qint64);
    void handleDownloadReadyRead();
    void handleDownloadFinished();
};

} // namespace dblsqd

#endif // DBLSQD_FEED_H
