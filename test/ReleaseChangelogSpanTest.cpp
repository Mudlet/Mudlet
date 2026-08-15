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
 * Covers which releases' notes the update offer shows
 * (UpdateDialog::generateChangelogDocument).
 *
 * The offer used to list the notes of the releases it could offer, which is not
 * the same set as the releases the user is about to receive: the 2026-08-07 PTB
 * published a Windows installer only, so a Linux user offered the 08-08 build
 * was never told what the 08-07 one changed, even though installing 08-08 hands
 * it to them. The span is therefore taken from the unfiltered release list,
 * bounded by what is running and what is on offer.
 */

namespace {
const auto aug06Tag = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-06-1a2b3c4d");
const auto aug07Tag = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-07-eaa991b9");
const auto aug08Tag = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-08-7c6b5a49");
const auto aug06Version = QStringLiteral("4.22.0-ptb-2026-08-06-1a2b3c4d");
const auto aug07Version = QStringLiteral("4.22.0-ptb-2026-08-07-eaa991b9");
const auto aug08Version = QStringLiteral("4.22.0-ptb-2026-08-08-7c6b5a49");

const auto windowsSuffix = QStringLiteral("-windows-64.exe");
const auto linuxSuffix = QStringLiteral("-linux-x64.AppImage.tar");

const auto linuxOs = QStringLiteral("linux");
const auto x64Arch = QStringLiteral("x86_64");

QJsonObject makeAsset(const QString& tag, const QString& name)
{
    QJsonObject asset;
    asset.insert(QStringLiteral("name"), name);
    asset.insert(QStringLiteral("browser_download_url"), QStringLiteral("https://github.com/Mudlet/Mudlet/releases/download/%1/%2").arg(tag, name));
    asset.insert(QStringLiteral("size"), 137252032);
    return asset;
}

QJsonObject releaseJson(const QString& tag, const QString& publishedAt, const QStringList& assetSuffixes, bool prerelease = true)
{
    QJsonArray assets;
    for (const auto& suffix : assetSuffixes) {
        assets.append(makeAsset(tag, QStringLiteral("%1%2").arg(tag, suffix)));
    }
    assets.append(makeAsset(tag, QStringLiteral("SHA256SUMS.txt")));

    QJsonObject release;
    release.insert(QStringLiteral("tag_name"), tag);
    release.insert(QStringLiteral("published_at"), publishedAt);
    release.insert(QStringLiteral("prerelease"), prerelease);
    release.insert(QStringLiteral("draft"), false);
    release.insert(QStringLiteral("body"), QStringLiteral("- %1 changed a thing\n").arg(tag));
    release.insert(QStringLiteral("assets"), assets);
    return release;
}

dblsqd::Release linuxRelease(const QJsonObject& json)
{
    return dblsqd::Release(json, linuxOs, x64Arch);
}

// Newest first, the order Feed sorts its releases into. Only the 08-07 build
// published a Windows installer, as the real one did.
QList<dblsqd::Release> feedReleases()
{
    return {linuxRelease(releaseJson(aug08Tag, QStringLiteral("2026-08-08T02:12:36Z"), {windowsSuffix, linuxSuffix})),
            linuxRelease(releaseJson(aug07Tag, QStringLiteral("2026-08-07T02:14:11Z"), {windowsSuffix})),
            linuxRelease(releaseJson(aug06Tag, QStringLiteral("2026-08-06T02:11:47Z"), {windowsSuffix, linuxSuffix}))};
}

// Release::getCurrentRelease() dates the running release by when it was built,
// which is always before the release carrying it was published
dblsqd::Release runningRelease(const QString& version, const QString& buildDate)
{
    return dblsqd::Release(version, QDateTime::fromString(buildDate, Qt::ISODate));
}

QStringList versionsOf(const QList<dblsqd::Release>& releases)
{
    QStringList versions;
    for (const auto& release : releases) {
        versions << release.getVersion();
    }
    return versions;
}
} // namespace

class ReleaseChangelogSpanTest : public QObject
{
    Q_OBJECT

private slots:
    void theNotesOfAReleaseThisPlatformWasNotOfferedAreStillShown();
    void whatIsAlreadyRunningAndOlderIsLeftOut();
    void theOfferedReleaseIsShownAndAnythingNewerIsNot();
    void nothingIsShownWhenNothingIsOffered();
    void releasesAreSpannedBySemVerOrder();
    void aVersionSemVerCannotReadIsSpannedByDateWithoutRepeatingItself();
};

// The regression: on Linux the 08-07 build is not offerable, but 08-08 carries
// its changes, so its notes have to appear alongside 08-08's
void ReleaseChangelogSpanTest::theNotesOfAReleaseThisPlatformWasNotOfferedAreStillShown()
{
    const auto releases = feedReleases();
    const auto running = runningRelease(aug06Version, QStringLiteral("2026-08-06T01:49:20Z"));
    const auto updates = dblsqd::Feed::selectUpdates(releases, running);
    QCOMPARE(versionsOf(updates), QStringList{aug08Version});

    const auto span = dblsqd::Feed::selectReleasesBetween(releases, running, updates.first());

    QCOMPARE(versionsOf(span), QStringList({aug08Version, aug07Version}));
    QVERIFY(span.at(1).getChangelog().contains(aug07Tag));
}

// The running release is in the feed too, and its notes were read long ago
void ReleaseChangelogSpanTest::whatIsAlreadyRunningAndOlderIsLeftOut()
{
    const auto releases = feedReleases();
    const auto running = runningRelease(aug07Version, QStringLiteral("2026-08-07T01:52:04Z"));

    const auto span = dblsqd::Feed::selectReleasesBetween(releases, running, releases.constFirst());

    QCOMPARE(versionsOf(span), QStringList{aug08Version});
}

// A release newer than the one on offer is not being installed, so promising
// its changes would be a lie - it can be newer because this platform has no
// asset in it, which is how the offer comes to be an older release. The offer
// itself is what the user is about to get, so it is shown.
void ReleaseChangelogSpanTest::theOfferedReleaseIsShownAndAnythingNewerIsNot()
{
    const auto releases = feedReleases();
    const auto running = runningRelease(QStringLiteral("4.22.0-ptb-2026-08-05-9f8e7d6c"), QStringLiteral("2026-08-05T01:47:31Z"));
    const auto offered = releases.constLast();
    QCOMPARE(offered.getVersion(), aug06Version);

    const auto span = dblsqd::Feed::selectReleasesBetween(releases, running, offered);

    QCOMPARE(versionsOf(span), QStringList{aug06Version});
}

void ReleaseChangelogSpanTest::nothingIsShownWhenNothingIsOffered()
{
    const auto running = runningRelease(aug06Version, QStringLiteral("2026-08-06T01:49:20Z"));

    QVERIFY(dblsqd::Feed::selectReleasesBetween(feedReleases(), running, dblsqd::Release()).isEmpty());
}

// Both stable and PTB versions are valid SemVer - the PTB date lives in the
// prerelease identifier - so the version, not the publication date, orders them
void ReleaseChangelogSpanTest::releasesAreSpannedBySemVerOrder()
{
    const QList<dblsqd::Release> releases{linuxRelease(releaseJson(QStringLiteral("Mudlet-4.22.0"), QStringLiteral("2026-07-06T09:00:00Z"), {linuxSuffix}, /*prerelease=*/false)),
                                          linuxRelease(releaseJson(QStringLiteral("Mudlet-4.21.1"), QStringLiteral("2026-05-11T09:00:00Z"), {linuxSuffix}, /*prerelease=*/false)),
                                          linuxRelease(releaseJson(QStringLiteral("Mudlet-4.21.0"), QStringLiteral("2026-04-27T09:00:00Z"), {linuxSuffix}, /*prerelease=*/false))};
    const dblsqd::Release running(QStringLiteral("4.21.0"));

    const auto span = dblsqd::Feed::selectReleasesBetween(releases, running, releases.constFirst());

    QCOMPARE(versionsOf(span), QStringList({QStringLiteral("4.22.0"), QStringLiteral("4.21.1")}));
}

// A two-component version is not SemVer, so these fall back to being ordered by
// date - and the running one is dated when it was built, which is before the
// release carrying it was published, so it looks newer than itself. Such a
// version never reaches a release (CI/check-release-tag.sh rejects it), but the
// span has to answer for one rather than replay notes the user already has.
void ReleaseChangelogSpanTest::aVersionSemVerCannotReadIsSpannedByDateWithoutRepeatingItself()
{
    const QList<dblsqd::Release> releases{linuxRelease(releaseJson(QStringLiteral("Mudlet-5.0-ptb-2026-08-08-7c6b5a49"), QStringLiteral("2026-08-08T02:12:36Z"), {linuxSuffix})),
                                          linuxRelease(releaseJson(QStringLiteral("Mudlet-5.0-ptb-2026-08-07-eaa991b9"), QStringLiteral("2026-08-07T02:14:11Z"), {linuxSuffix}))};
    const auto running = runningRelease(QStringLiteral("5.0-ptb-2026-08-07-eaa991b9"), QStringLiteral("2026-08-07T01:52:04Z"));

    const auto span = dblsqd::Feed::selectReleasesBetween(releases, running, releases.constFirst());

    QCOMPARE(versionsOf(span), QStringList{QStringLiteral("5.0-ptb-2026-08-08-7c6b5a49")});
}

#include "ReleaseChangelogSpanTest.moc"
QTEST_MAIN(ReleaseChangelogSpanTest)
