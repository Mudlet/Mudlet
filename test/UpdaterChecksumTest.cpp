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
 * Covers the pairing of a release's assets with its SHA256SUMS.txt, which is what
 * decides whether an update can be installed at all.
 *
 * Windows auto-update broke on the 2026-08-01 PTB: the release carried five
 * binaries but a SHA256SUMS.txt covering only four, and the uncovered one was the
 * .exe the Windows updater picks. Feed refuses to install a download it cannot
 * verify, so users got "Could not verify the integrity of the download" and no
 * update. The data below is that release's real asset list and checksum file.
 *
 * The publishing side of the fix - never overwriting SHA256SUMS.txt with a file
 * that covers fewer binaries - is covered by test/ci/release-checksums-test.sh.
 */

namespace {
const auto tag = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-01-dfdcb137");
const auto windowsAsset = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-01-dfdcb137-windows-64.exe");
const auto linuxAsset = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-01-dfdcb137-linux-x64.AppImage.tar");
const auto arm64Asset = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-01-dfdcb137-arm64.dmg");
const auto x86_64Asset = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-01-dfdcb137-x86_64.dmg");
// A re-run of the Windows build restamped the date and appended a rebuild counter
const auto rebuiltWindowsAsset = QStringLiteral("Mudlet-4.22.0-ptb-2026-08-02-dfdcb137rebuild2-windows-64.exe");

const auto windowsHash = QStringLiteral("72dba076741a245a994b553b1cf88dba5d7f5bb07f0d6887ecd58361402764e1");
const auto linuxHash = QStringLiteral("72a239146b07fc3b94f9e96955d2d6d52a6f0f40c8a5b7ee314b0bf875a55611");
const auto arm64Hash = QStringLiteral("7a7a75723e15a3443c4e8937fe2a85cee90b9dc8938e1e0580887e5c5e1fabbb");
const auto x86_64Hash = QStringLiteral("d2426d7f799b619b0bfcb1e51e373f776288f77584bc9f08a79cc288cc58278c");
const auto rebuiltWindowsHash = QStringLiteral("9b3895247937d37f645dd31e7ded739e3126ccad6bfa4188cdd3b78f735c2f6f");

// Exactly what the 2026-08-01 PTB shipped: the .exe on the release is absent and a
// different build's .exe is listed in its place
QString publishedChecksums()
{
    return QStringLiteral("%1  %2\n%3  %4\n%5 *%6\n%7  %8\n").arg(linuxHash, linuxAsset, x86_64Hash, x86_64Asset, rebuiltWindowsHash, rebuiltWindowsAsset, arm64Hash, arm64Asset);
}

// What the release should have shipped, and does once the publishing scripts merge
// instead of overwrite
QString mergedChecksums()
{
    return publishedChecksums() + QStringLiteral("%1 *%2\n").arg(windowsHash, windowsAsset);
}

// The 2026-08-01 PTB's assets, in the order the GitHub releases API returns them
QJsonObject releaseJson()
{
    const QStringList assetNames{arm64Asset, linuxAsset, windowsAsset, x86_64Asset, rebuiltWindowsAsset, QStringLiteral("SHA256SUMS.txt")};

    QJsonArray assets;
    for (const auto& name : assetNames) {
        QJsonObject asset;
        asset.insert(QStringLiteral("name"), name);
        asset.insert(QStringLiteral("browser_download_url"), QStringLiteral("https://github.com/Mudlet/Mudlet/releases/download/%1/%2").arg(tag, name));
        asset.insert(QStringLiteral("size"), 137252032);
        assets.append(asset);
    }

    QJsonObject release;
    release.insert(QStringLiteral("tag_name"), tag);
    release.insert(QStringLiteral("published_at"), QStringLiteral("2026-07-31T18:22:33Z"));
    release.insert(QStringLiteral("prerelease"), true);
    release.insert(QStringLiteral("draft"), false);
    release.insert(QStringLiteral("assets"), assets);
    return release;
}
} // namespace

class UpdaterChecksumTest : public QObject
{
    Q_OBJECT

private slots:
    void windowsDownloadIsThePlatformExe();
    void publishedChecksumsDoNotCoverTheWindowsDownload();
    void mergedChecksumsCoverTheWindowsDownload();
    void everyOtherPlatformWasAlreadyCovered();
    void binaryAndTextModeLinesBothParse();
    void anotherBuildsEntryDoesNotCoverThisDownload();
    void malformedLinesAreIgnored();
    void emptyInputsYieldNoChecksum();
};

// The updater picks the first asset matching its platform, so this is the file
// whose checksum has to be present
void UpdaterChecksumTest::windowsDownloadIsThePlatformExe()
{
    const dblsqd::Release release(releaseJson(), QStringLiteral("win"), QStringLiteral("x86_64"));

    QCOMPARE(release.getDownloadUrl().fileName(), windowsAsset);
    QCOMPARE(release.getChecksumsUrl().fileName(), QStringLiteral("SHA256SUMS.txt"));
}

// The regression: this is why Windows auto-update failed
void UpdaterChecksumTest::publishedChecksumsDoNotCoverTheWindowsDownload()
{
    const dblsqd::Release release(releaseJson(), QStringLiteral("win"), QStringLiteral("x86_64"));

    QVERIFY(dblsqd::Feed::findChecksum(publishedChecksums(), release.getDownloadUrl().fileName()).isEmpty());
}

void UpdaterChecksumTest::mergedChecksumsCoverTheWindowsDownload()
{
    const dblsqd::Release release(releaseJson(), QStringLiteral("win"), QStringLiteral("x86_64"));

    QCOMPARE(dblsqd::Feed::findChecksum(mergedChecksums(), release.getDownloadUrl().fileName()), windowsHash);
}

void UpdaterChecksumTest::everyOtherPlatformWasAlreadyCovered()
{
    const dblsqd::Release linuxRelease(releaseJson(), QStringLiteral("linux"), QStringLiteral("x86_64"));
    QCOMPARE(linuxRelease.getDownloadUrl().fileName(), linuxAsset);
    QCOMPARE(dblsqd::Feed::findChecksum(publishedChecksums(), linuxRelease.getDownloadUrl().fileName()), linuxHash);

    const dblsqd::Release intelMacRelease(releaseJson(), QStringLiteral("mac"), QStringLiteral("x86_64"));
    QCOMPARE(intelMacRelease.getDownloadUrl().fileName(), x86_64Asset);
    QCOMPARE(dblsqd::Feed::findChecksum(publishedChecksums(), intelMacRelease.getDownloadUrl().fileName()), x86_64Hash);

    const dblsqd::Release appleSiliconRelease(releaseJson(), QStringLiteral("mac"), QStringLiteral("arm64"));
    QCOMPARE(appleSiliconRelease.getDownloadUrl().fileName(), arm64Asset);
    QCOMPARE(dblsqd::Feed::findChecksum(publishedChecksums(), appleSiliconRelease.getDownloadUrl().fileName()), arm64Hash);
}

// sha256sum writes two spaces in text mode and " *" in binary mode; the Windows
// build produces the latter
void UpdaterChecksumTest::binaryAndTextModeLinesBothParse()
{
    QCOMPARE(dblsqd::Feed::findChecksum(QStringLiteral("%1 *%2").arg(windowsHash, windowsAsset), windowsAsset), windowsHash);
    QCOMPARE(dblsqd::Feed::findChecksum(QStringLiteral("%1  %2").arg(windowsHash, windowsAsset), windowsAsset), windowsHash);
    QCOMPARE(dblsqd::Feed::findChecksum(QStringLiteral("%1\t%2").arg(windowsHash, windowsAsset), windowsAsset), windowsHash);
    // trailing CR from a file written on Windows must not become part of the name
    QCOMPARE(dblsqd::Feed::findChecksum(QStringLiteral("%1 *%2\r\n").arg(windowsHash, windowsAsset), windowsAsset), windowsHash);
}

// The rebuilt installer's entry must not be accepted for a different file, or the
// updater would check the download against the wrong hash
void UpdaterChecksumTest::anotherBuildsEntryDoesNotCoverThisDownload()
{
    const QString rebuiltOnly = QStringLiteral("%1 *%2\n").arg(rebuiltWindowsHash, rebuiltWindowsAsset);

    QVERIFY(dblsqd::Feed::findChecksum(rebuiltOnly, windowsAsset).isEmpty());
    QCOMPARE(dblsqd::Feed::findChecksum(rebuiltOnly, rebuiltWindowsAsset), rebuiltWindowsHash);
}

void UpdaterChecksumTest::malformedLinesAreIgnored()
{
    // too short, non-hex, and no separator respectively, then the real entry
    const QString data = QStringLiteral("abc123  %1\n"
                                        "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz  %1\n"
                                        "%2\n"
                                        "%3 *%1\n")
                                 .arg(windowsAsset, windowsHash, windowsHash);

    QCOMPARE(dblsqd::Feed::findChecksum(data, windowsAsset), windowsHash);
}

void UpdaterChecksumTest::emptyInputsYieldNoChecksum()
{
    QVERIFY(dblsqd::Feed::findChecksum(QString(), windowsAsset).isEmpty());
    QVERIFY(dblsqd::Feed::findChecksum(mergedChecksums(), QString()).isEmpty());
}

#include "UpdaterChecksumTest.moc"
QTEST_MAIN(UpdaterChecksumTest)
