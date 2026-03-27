#ifndef DBLSQD_RELEASE_H
#define DBLSQD_RELEASE_H

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QUrl>

namespace dblsqd {

class Release
{
public:
    Release(const QJsonObject& releaseInfo, const QString& os = QString(), const QString& arch = QString());
    Release(const QString& version = QString(), const QDateTime& date = QDateTime());

    friend bool operator<(const Release& one, const Release& other);
    friend bool operator==(const Release& one, const Release& other);
    friend bool operator<=(const Release& one, const Release& other);

    QString getVersion() const;
    QString getChangelog() const;
    QDateTime getDate() const;
    QUrl getDownloadUrl() const;
    QString getDownloadSHA256() const;
    qint64 getDownloadSize() const;
    QUrl getChecksumsUrl() const;
    void setDownloadSHA256(const QString& sha256);
    static dblsqd::Release getCurrentRelease();

private:
    QString mVersion;
    QDateTime mDate;
    QString mChangelog;
    QUrl mDownloadUrl;
    qint64 mDownloadSize{0};
    QString mDownloadSHA256;
    QUrl mChecksumsUrl;

    static QString buildAssetPattern(const QString& os, const QString& arch);
};

} // namespace dblsqd

#endif // DBLSQD_RELEASE_H
