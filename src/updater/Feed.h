/***************************************************************************
 *   Copyright (C) 2017 by Philipp Medien - hello@dblsqd.com               *
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@gmail.com          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef DBLSQD_FEED_H
#define DBLSQD_FEED_H

#include "Release.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>

class QNetworkReply;
class QTemporaryFile;

// Namespace retained from the previous dblsqd update system to minimize changes in dependent code
namespace dblsqd {

class Feed : public QObject
{
    Q_OBJECT

public:
    explicit Feed(QObject* parent = nullptr);
    ~Feed();

    void setRepo(const QString& owner, const QString& repo, bool prerelease = false, const QString& os = QString(), const QString& arch = QString());
    QUrl getUrl() const;
    QString getOwner() const;
    QString getRepo() const;

    void load();
    void downloadRelease(const Release& release, bool requireChecksums = true);

    // Returns the SHA256 that sha256sum-style output lists for downloadFilename,
    // comparing the whole filename case-insensitively, or an empty string when no
    // line covers it. entriesParsed, when given, receives the number of well-formed
    // lines seen, which tells "this release forgot my platform" apart from "that was
    // not a checksum file".
    static QString findChecksum(const QString& checksumData, const QString& downloadFilename, int* entriesParsed = nullptr);

    // The releases newer than currentRelease that this platform can install. A
    // release with no asset for this platform - a build job that failed, or
    // assets that are still uploading - or no SHA256SUMS.txt to verify the
    // download against cannot be installed, so offering it only produces a
    // download error the user can do nothing about. The changelog is built from
    // getReleases() instead, and still covers them.
    QList<Release> getUpdates(const Release& currentRelease) const;
    static QList<Release> selectUpdates(const QList<Release>& releases, const Release& currentRelease);

    // The releases in (after, upTo] - everything installing upTo brings with it.
    // Pass the unfiltered release list: a release getUpdates() passed over for
    // want of an asset for this platform is still part of what a later release
    // delivers, so its notes belong in the changelog for it.
    static QList<Release> selectReleasesBetween(const QList<Release>& releases, const Release& after, const Release& upTo);

    QList<Release> getReleases() const;
    QString getDownloadFilePath() const;
    bool isReady() const;
    bool isDownloading() const;

signals:
    void ready();
    void loadError(const QString& message);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    void downloadError(const QString& message);

private:
    QUrl mUrl;

    QList<Release> mReleases;

    void makeDownloadRequest(const QUrl& url);
    void fetchChecksums(const QUrl& checksumsUrl);
    void abortDownload();
    void cleanupDownloadReply();
    void cleanupDownloadFile();
    bool mRequireChecksums{false};

    QNetworkAccessManager mNam;
    QNetworkReply* mFeedReply{nullptr};
    QNetworkReply* mChecksumsReply{nullptr};
    Release mCurrentDownload;
    QNetworkReply* mDownloadReply{nullptr};
    QTemporaryFile* mDownloadFile{nullptr};
    QString mDownloadFilePath;
    bool mReady{false};

    QString mOwner;
    QString mRepo;
    bool mPrerelease{false};
    QString mOs;
    QString mArch;

    static QString detectOs();
    static QString detectArch();

private slots:
    void handleFeedFinished();
    void handleDownloadReadyRead();
    void handleDownloadFinished();
};

} // namespace dblsqd

#endif // DBLSQD_FEED_H
