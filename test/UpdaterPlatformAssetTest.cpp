/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org     *
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

#include "../src/updater/Feed.h"
#include "../src/updater/Release.h"

#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonObject>

/*
 * Covers which releases the updater is willing to offer as an update.
 *
 * The 2026-08-07 PTB published a Windows installer and nothing else: the Linux
 * and macOS build jobs failed, so their assets were never attached. Linux PTB
 * users got a red "no download available for your platform" error on the
 * console on every check, because the release was offered as an update and the
 * download then had nothing to fetch. A partly published release is valid and
 * will happen again, so it has to be passed over instead - as does one that
 * published a binary but no SHA256SUMS.txt, since the download refuses to
 * install what it cannot verify.
 */

namespace {
// Only windowsOnlyTag is a real release; the complete, unverifiable and
// installed ones are constructed around it
const auto windowsOnlyTag = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-07-eaa991b9");
const auto unverifiableTag = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-07-5e4d3c2b");
const auto completeTag = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-06-1a2b3c4d");
const auto windowsOnlyVersion = QStringLiteral("4.22.0-ptb-2026-08-07-eaa991b9");
const auto completeVersion = QStringLiteral("4.22.0-ptb-2026-08-06-1a2b3c4d");
const auto installedVersion = QStringLiteral("4.22.0-ptb-2026-08-05-9f8e7d6c");

const auto windowsSuffix = QStringLiteral("-windows-64.exe");
const auto linuxSuffix = QStringLiteral("-linux-x64.AppImage.tar");
const auto intelMacSuffix = QStringLiteral("-x86_64.dmg");
const auto appleSiliconSuffix = QStringLiteral("-arm64.dmg");

QJsonObject makeAsset(const QString& tag, const QString& name)
{
    QJsonObject asset;
    asset.insert(QStringLiteral("name"), name);
    asset.insert(QStringLiteral("browser_download_url"), QStringLiteral("https://github.com/Mudlet/Mudlet/releases/download/%1/%2").arg(tag, name));
    asset.insert(QStringLiteral("size"), 137252032);
    return asset;
}

QJsonObject releaseJson(const QString& tag, const QString& publishedAt, const QStringList& assetSuffixes, bool withChecksums = true)
{
    QJsonArray assets;
    for (const auto& suffix : assetSuffixes) {
        assets.append(makeAsset(tag, QStringLiteral("%1%2").arg(tag, suffix)));
    }
    if (withChecksums) {
        assets.append(makeAsset(tag, QStringLiteral("SHA256SUMS.txt")));
    }

    QJsonObject release;
    release.insert(QStringLiteral("tag_name"), tag);
    release.insert(QStringLiteral("published_at"), publishedAt);
    release.insert(QStringLiteral("prerelease"), true);
    release.insert(QStringLiteral("draft"), false);
    release.insert(QStringLiteral("body"), QStringLiteral("- fixed a thing\n"));
    release.insert(QStringLiteral("assets"), assets);
    return release;
}

QJsonObject windowsOnlyRelease()
{
    return releaseJson(windowsOnlyTag, QStringLiteral("2026-08-07T02:14:11Z"), {windowsSuffix});
}

// Published its Linux binary, but not the checksums that binary is verified against
QJsonObject unverifiableRelease()
{
    return releaseJson(unverifiableTag, QStringLiteral("2026-08-07T04:22:09Z"), {linuxSuffix}, /*withChecksums=*/false);
}

QJsonObject completeRelease()
{
    return releaseJson(completeTag, QStringLiteral("2026-08-06T02:11:47Z"), {windowsSuffix, linuxSuffix, intelMacSuffix, appleSiliconSuffix});
}

// Newest first, the order Feed sorts its releases into
QList<dblsqd::Release> feedReleases(const QString& os, const QString& arch)
{
    return {dblsqd::Release(windowsOnlyRelease(), os, arch), dblsqd::Release(completeRelease(), os, arch)};
}

dblsqd::Release installedRelease()
{
    return dblsqd::Release(installedVersion, QDateTime::fromString(QStringLiteral("2026-08-05T02:09:03Z"), Qt::ISODate));
}
} // namespace

class UpdaterPlatformAssetTest : public QObject
{
    Q_OBJECT

private slots:
    void linuxIsNotOfferedTheReleaseWithoutALinuxAsset();
    void windowsIsStillOfferedIt();
    void intelMacIsNotOfferedTheReleaseWithoutADmg();
    void appleSiliconIsNotOfferedTheReleaseWithoutADmg();
    void aReleaseWithOnlyItsChecksumsFileIsNotOffered();
    void aReleaseWithoutChecksumsIsNotOffered();
    void theVerifiableReleaseIsOfferedInsteadOfTheNewerUnverifiableOne();
    void aPlatformWithNoAssetsAtAllIsOfferedNothing();
    void nothingIsOfferedOnceTheOnlyNewerReleaseIsIncomplete();
    void thePassedOverReleaseStaysReadableForTheChangelog();
};

// The regression: this is the update that produced a red console error twice a day
void UpdaterPlatformAssetTest::linuxIsNotOfferedTheReleaseWithoutALinuxAsset()
{
    const auto updates = dblsqd::Feed::selectUpdates(feedReleases(QStringLiteral("linux"), QStringLiteral("x86_64")), installedRelease());

    QCOMPARE(updates.size(), 1);
    QCOMPARE(updates.first().getVersion(), completeVersion);
    QVERIFY(updates.first().getDownloadUrl().fileName().endsWith(linuxSuffix));
}

void UpdaterPlatformAssetTest::windowsIsStillOfferedIt()
{
    const auto updates = dblsqd::Feed::selectUpdates(feedReleases(QStringLiteral("win"), QStringLiteral("x86_64")), installedRelease());

    QCOMPARE(updates.size(), 2);
    QCOMPARE(updates.first().getVersion(), windowsOnlyVersion);
    QCOMPARE(updates.last().getVersion(), completeVersion);
}

void UpdaterPlatformAssetTest::intelMacIsNotOfferedTheReleaseWithoutADmg()
{
    const auto updates = dblsqd::Feed::selectUpdates(feedReleases(QStringLiteral("mac"), QStringLiteral("x86_64")), installedRelease());

    QCOMPARE(updates.size(), 1);
    QCOMPARE(updates.first().getVersion(), completeVersion);
    QVERIFY(updates.first().getDownloadUrl().fileName().endsWith(intelMacSuffix));
}

void UpdaterPlatformAssetTest::appleSiliconIsNotOfferedTheReleaseWithoutADmg()
{
    const auto updates = dblsqd::Feed::selectUpdates(feedReleases(QStringLiteral("mac"), QStringLiteral("arm64")), installedRelease());

    QCOMPARE(updates.size(), 1);
    QCOMPARE(updates.first().getVersion(), completeVersion);
    QVERIFY(updates.first().getDownloadUrl().fileName().endsWith(appleSiliconSuffix));
}

// A release whose binaries are still uploading looks the same as one that lost a build job
void UpdaterPlatformAssetTest::aReleaseWithOnlyItsChecksumsFileIsNotOffered()
{
    const QList<dblsqd::Release> releases{dblsqd::Release(releaseJson(windowsOnlyTag, QStringLiteral("2026-08-07T02:14:11Z"), {}), QStringLiteral("linux"), QStringLiteral("x86_64"))};

    QVERIFY(dblsqd::Feed::selectUpdates(releases, installedRelease()).isEmpty());
}

// The download refuses to install what it cannot verify, so a release whose
// SHA256SUMS.txt is missing is as uninstallable as one missing its binary
void UpdaterPlatformAssetTest::aReleaseWithoutChecksumsIsNotOffered()
{
    const QList<dblsqd::Release> releases{dblsqd::Release(unverifiableRelease(), QStringLiteral("linux"), QStringLiteral("x86_64"))};

    QVERIFY(dblsqd::Feed::selectUpdates(releases, installedRelease()).isEmpty());
}

void UpdaterPlatformAssetTest::theVerifiableReleaseIsOfferedInsteadOfTheNewerUnverifiableOne()
{
    const QList<dblsqd::Release> releases{dblsqd::Release(unverifiableRelease(), QStringLiteral("linux"), QStringLiteral("x86_64")),
                                          dblsqd::Release(completeRelease(), QStringLiteral("linux"), QStringLiteral("x86_64"))};

    const auto updates = dblsqd::Feed::selectUpdates(releases, installedRelease());

    QCOMPARE(updates.size(), 1);
    QCOMPARE(updates.first().getVersion(), completeVersion);
}

// Mudlet publishes no binaries for the platforms it is packaged for by others,
// so those builds are told there is no update rather than shown a failure they
// can do nothing about, twice a day, forever
void UpdaterPlatformAssetTest::aPlatformWithNoAssetsAtAllIsOfferedNothing()
{
    QVERIFY(dblsqd::Feed::selectUpdates(feedReleases(QStringLiteral("freebsd"), QStringLiteral("x86_64")), installedRelease()).isEmpty());
}

// What a Linux user on the previous PTB sees: no update, rather than an error
void UpdaterPlatformAssetTest::nothingIsOfferedOnceTheOnlyNewerReleaseIsIncomplete()
{
    const QList<dblsqd::Release> releases{dblsqd::Release(windowsOnlyRelease(), QStringLiteral("linux"), QStringLiteral("x86_64"))};
    const dblsqd::Release installed(completeVersion, QDateTime::fromString(QStringLiteral("2026-08-06T02:11:47Z"), Qt::ISODate));

    QVERIFY(dblsqd::Feed::selectUpdates(releases, installed).isEmpty());
}

// A release passed over here still has to carry its version and notes: the
// changelog dialogs render the unfiltered release list
// (UpdateDialog::generateChangelogDocument)
void UpdaterPlatformAssetTest::thePassedOverReleaseStaysReadableForTheChangelog()
{
    const dblsqd::Release release(windowsOnlyRelease(), QStringLiteral("linux"), QStringLiteral("x86_64"));

    QVERIFY(release.getDownloadUrl().isEmpty());
    QCOMPARE(release.getVersion(), windowsOnlyVersion);
    QCOMPARE(release.getChangelog(), QStringLiteral("- fixed a thing\n"));
}

#include "UpdaterPlatformAssetTest.moc"
QTEST_MAIN(UpdaterPlatformAssetTest)
