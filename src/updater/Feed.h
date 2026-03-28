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

namespace dblsqd {

class Feed : public QObject
{
    Q_OBJECT

public:
    explicit Feed(QObject* parent = nullptr);
    ~Feed();

    void setRepo(const QString& owner, const QString& repo, bool prerelease = false, const QString& os = QString(), const QString& arch = QString());
    QUrl getUrl() const;

    void load();
    void downloadRelease(const Release& release, bool requireChecksums = false);

    QList<Release> getUpdates(const Release& currentRelease) const;
    QList<Release> getReleases() const;
    QString getDownloadFilePath() const;
    bool isReady() const;

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
