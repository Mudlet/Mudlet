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

#include "Release.h"

#include "SemVer.h"

#include "../utils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextDocument>

namespace dblsqd {

/*!
 * \class Release
 * \brief This class is used to represent information about a single Release
 * from a Feed.
 */

/*!
 * \brief Constructs a new Release from a GitHub Releases API JSON object.
 */
Release::Release(const QJsonObject& releaseInfo, const QString& os, const QString& arch)
{
    // Version: strip "Mudlet-" prefix from tag_name
    const QString tagName = releaseInfo.value(qsl("tag_name")).toString();
    mVersion = tagName.startsWith(qsl("Mudlet-")) ? tagName.mid(7) : tagName;

    mDate = QDateTime::fromString(releaseInfo.value(qsl("published_at")).toString(), Qt::ISODate);

    // Changelog: convert GitHub markdown body to HTML
    const QString body = releaseInfo.value(qsl("body")).toString();
    if (!body.isEmpty()) {
        QTextDocument doc;
        doc.setMarkdown(body);
        mChangelog = doc.toHtml();
    }

    // Find matching asset by platform/arch
    const QJsonArray assets = releaseInfo.value(qsl("assets")).toArray();
    const QString assetPattern = buildAssetPattern(os, arch);

    for (const auto& assetVal : assets) {
        const QJsonObject asset = assetVal.toObject();
        const QString name = asset.value(qsl("name")).toString();

        // Look for SHA256SUMS.txt checksums file
        if (name == qsl("SHA256SUMS.txt")) {
            mChecksumsUrl = QUrl(asset.value(qsl("browser_download_url")).toString());
            continue;
        }

        // Match platform-specific asset
        if (!assetPattern.isEmpty() && name.contains(assetPattern, Qt::CaseInsensitive)) {
            mDownloadUrl = QUrl(asset.value(qsl("browser_download_url")).toString());
            mDownloadSize = static_cast<qint64>(asset.value(qsl("size")).toDouble());
        }
    }

    if (mDownloadUrl.isEmpty() && !mVersion.isEmpty() && !os.isEmpty()) {
        qWarning() << "No matching asset found for" << os << arch << "in release" << mVersion;
    }
}

/*!
 * \brief Constructs a new Release from a version string and a date.
 *
 * This method is useful when constructing a "virtual" Release for comparing
 * it with Releases retrieved from a Feed.
 */
Release::Release(const QString& version, const QDateTime& date)
: mVersion(version)
, mDate(date)
{
}

bool operator<(const Release& one, const Release& other)
{
    SemVer v1(one.mVersion);
    SemVer v2(other.mVersion);

    // Both valid SemVer: use semantic version comparison
    if (v1.isValid() && v2.isValid()) {
        return (v1 < v2);
    }

    // Mixed validity: valid SemVer sorts after invalid to ensure total ordering
    // (required by std::sort's strict weak ordering contract)
    if (v1.isValid() != v2.isValid()) {
        return v2.isValid();
    }

    // Both invalid SemVer (e.g. PTB releases): fall back to date comparison
    return (one.mDate < other.mDate);
}

bool operator==(const Release& one, const Release& other)
{
    return one.mVersion == other.mVersion;
}

bool operator<=(const Release& one, const Release& other)
{
    return one == other || one < other;
}

QString Release::getVersion() const
{
    return mVersion;
}

QString Release::getChangelog() const
{
    return mChangelog;
}

QDateTime Release::getDate() const
{
    return mDate;
}

QUrl Release::getDownloadUrl() const
{
    return mDownloadUrl;
}

QString Release::getDownloadSHA256() const
{
    return mDownloadSHA256;
}

void Release::setDownloadSHA256(const QString& sha256)
{
    mDownloadSHA256 = sha256;
}

qint64 Release::getDownloadSize() const
{
    return mDownloadSize;
}

QUrl Release::getChecksumsUrl() const
{
    return mChecksumsUrl;
}

dblsqd::Release Release::getCurrentRelease()
{
    // embed build time so public test releases, which cannot be compared via semver, can be compared via datetime
    QString buildDateTime = QString(__DATE__) + " " + QString(__TIME__);
    // locale-independent datetime parsing (C locale matches __DATE__'s English format)
    QDateTime date = QLocale::c().toDateTime(buildDateTime.simplified(), qsl("MMM d yyyy hh:mm:ss"));

    return dblsqd::Release(QCoreApplication::applicationVersion(), date);
}

QString Release::buildAssetPattern(const QString& os, const QString& arch)
{
    if (os == qsl("linux")) {
        return qsl("-linux-x64.AppImage.tar");
    } else if (os == qsl("win")) {
        return qsl("-windows-64-installer.exe");
    } else if (os == qsl("mac")) {
        if (arch == qsl("arm64") || arch == qsl("aarch64")) {
            return qsl("-arm64.dmg");
        }
        return qsl("-x86_64.dmg");
    }
    return QString();
}

} // namespace dblsqd
