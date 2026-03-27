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

#ifndef DBLSQD_RELEASE_H
#define DBLSQD_RELEASE_H

#include <QDateTime>
#include <QString>
#include <QUrl>

class QJsonObject;

namespace dblsqd {

class Release
{
public:
    explicit Release(const QJsonObject& releaseInfo, const QString& os = QString(), const QString& arch = QString());
    explicit Release(const QString& version = QString(), const QDateTime& date = QDateTime());

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
