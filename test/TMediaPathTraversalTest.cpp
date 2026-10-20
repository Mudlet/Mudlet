/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TMedia.h"

#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

/*
 * Regression guard for the media path-traversal fix.
 *
 * A remote game server (or a script) supplies the media file name used by MSP,
 * MXP, GMCP Client.Media.Load/Play and the Lua media API. The original
 * containment check was "is the path relative?", which accepts "../" because a
 * traversal string is still relative. That let a server escape the profile media
 * directory and have downloaded, attacker-controlled content written anywhere
 * the process could write (arbitrary file write), and read arbitrary local files
 * back for playback.
 *
 * TMedia::mediaFilePathEscapesMediaDir() is the containment check every
 * download/write and read/playback path funnels through. It resolves the file
 * name against the media directory and returns true only when the result escapes
 * it - so legitimate sub-directories and wildcards keep working while traversal
 * is refused. The first (lexical) layer is platform-independent and needs no
 * files on disk; the second (canonical) layer defeats symlink components and is
 * exercised with a real temporary directory below.
 */
class TMediaPathTraversalTest : public QObject
{
    Q_OBJECT

    // A representative, absolute profile media directory. Its existence is
    // irrelevant for the lexical cases - the first-layer check is purely textual.
    static QString mediaRoot() { return QStringLiteral("/home/user/.config/mudlet/profiles/Default/media"); }

    static bool escapes(const QString& fileName) { return TMedia::mediaFilePathEscapesMediaDir(mediaRoot(), fileName); }

    static QString profileHome() { return QStringLiteral("/home/user/.config/mudlet/profiles/Default"); }

    static QString cacheName(const QString& absolutePath) { return TMedia::mediaCacheFileName(profileHome(), mediaRoot(), absolutePath); }

private slots:

    // -------------------------------------------------------------------------
    // Legitimate names must be permitted (no false positives).
    // -------------------------------------------------------------------------

    void benign_topLevelFile_allowed() { QVERIFY(!escapes(QStringLiteral("beep.wav"))); }

    void benign_subDirFile_allowed() { QVERIFY(!escapes(QStringLiteral("sounds/beep.wav"))); }

    void benign_nestedSubDirFile_allowed() { QVERIFY(!escapes(QStringLiteral("a/b/c/beep.wav"))); }

    void benign_wildcard_allowed() { QVERIFY(!escapes(QStringLiteral("*.wav"))); }

    void benign_wildcardInSubDir_allowed() { QVERIFY(!escapes(QStringLiteral("sounds/*.wav"))); }

    void benign_dotSlash_allowed() { QVERIFY(!escapes(QStringLiteral("./beep.wav"))); }

    // "../" that stays inside the media directory is harmless and must not be blocked.
    void benign_internalDotDot_allowed() { QVERIFY(!escapes(QStringLiteral("sounds/../beep.wav"))); }

    void empty_allowed() { QVERIFY(!escapes(QString())); }

    // -------------------------------------------------------------------------
    // Traversal must be refused (the vulnerability).
    // -------------------------------------------------------------------------

    void traversal_singleLevel_refused() { QVERIFY(escapes(QStringLiteral("../evil.wav"))); }

    void traversal_deep_refused() { QVERIFY(escapes(QStringLiteral("../../../../../../etc/passwd"))); }

    // The classic payload: a media-looking name that lands in the user's home.
    void traversal_intoHome_refused() { QVERIFY(escapes(QStringLiteral("../../../../.config/mudlet/evil.wav"))); }

    // Escapes to a sibling of the media directory (note: same prefix, different dir).
    void traversal_siblingDir_refused() { QVERIFY(escapes(QStringLiteral("../media_other/evil.wav"))); }

    // Dives into a sub-directory, then climbs out past the media root.
    void traversal_subDirThenOut_refused() { QVERIFY(escapes(QStringLiteral("sounds/../../evil.wav"))); }

    // -------------------------------------------------------------------------
    // Symlink containment - a lexical check alone is bypassable by a symlink that
    // lives inside the media directory but points outside it. The canonical layer
    // must catch it, while a genuine sub-directory of the media root stays allowed.
    // -------------------------------------------------------------------------

    void symlink_escapingMediaDir_refused()
    {
        QTemporaryDir base;
        QVERIFY(base.isValid());

        const QString root = base.filePath(QStringLiteral("media"));
        const QString outside = base.filePath(QStringLiteral("outside"));
        QVERIFY(QDir().mkpath(root));
        QVERIFY(QDir().mkpath(outside));

        // A symlink inside the media directory pointing outside it.
        const QString link = root + QStringLiteral("/escape");
        if (!QFile::link(outside, link)) {
            QSKIP("Filesystem does not support symlinks");
        }

        // Only meaningful where the platform created a *followed* symlink: on Windows
        // QFile::link() writes a .lnk shortcut that the filesystem does not traverse, so
        // there is no escape to catch there. Confirm the link actually redirects to the
        // outside directory at the canonical level before asserting.
        const QString linkCanonical = QFileInfo(link).canonicalFilePath();
        const QString outsideCanonical = QFileInfo(outside).canonicalFilePath();
        if (linkCanonical.isEmpty() || linkCanonical != outsideCanonical) {
            QSKIP("Filesystem did not create a followed symlink (e.g. Windows .lnk shortcut)");
        }

        // A path beneath the escaping symlink passes the lexical prefix test but must be
        // refused by the canonical layer.
        QVERIFY(TMedia::mediaFilePathEscapesMediaDir(root, QStringLiteral("escape/evil.wav")));
    }

    void symlink_realSubDir_allowed()
    {
        QTemporaryDir base;
        QVERIFY(base.isValid());

        const QString root = base.filePath(QStringLiteral("media"));
        QVERIFY(QDir().mkpath(root + QStringLiteral("/sounds")));

        // A genuine sub-directory of the media root must remain permitted.
        QVERIFY(!TMedia::mediaFilePathEscapesMediaDir(root, QStringLiteral("sounds/ok.wav")));
    }

    // A drive that is not there has no parent to walk on to; the walk must end, not spin on "Z:/"
    void resolve_missingDrive_ends()
    {
        QCOMPARE(TMedia::resolvedMediaPath(QStringLiteral("Z:/no-such-drive/tone.wav")), QStringLiteral("Z:/no-such-drive/tone.wav"));
        // On Windows "/" is the current drive's root, so the drive is named in the answer
        QCOMPARE(TMedia::resolvedMediaPath(QStringLiteral("/no-such-folder/tone.wav")), QDir(QFileInfo(QStringLiteral("/")).canonicalFilePath()).filePath(QStringLiteral("no-such-folder/tone.wav")));
    }

    // A share that is offline is a root too, not a folder of the current drive's root
    void resolve_missingShare_ends()
    {
        const QString share = QStringLiteral("//no-such-server/share/tone.wav");
        QCOMPARE(TMedia::resolvedMediaPath(share), QDir::cleanPath(share));
    }

    // Only a drive letter makes a drive: a folder whose name ends in ':' is walked up like any other
    void resolve_folderEndingInColon_isAFolder()
    {
        QTemporaryDir base;
        QVERIFY(base.isValid());
        const QString real = base.filePath(QStringLiteral("real"));
        QVERIFY(QDir().mkpath(real));

        QCOMPARE(TMedia::resolvedMediaPath(real + QStringLiteral("/snd:/gone.wav")), QFileInfo(real).canonicalFilePath() + QStringLiteral("/snd:/gone.wav"));
    }

    // A file that has gone resolves through the folders that remain, symlinks among them
    void resolve_missingFileUnderSymlink_realFolder()
    {
        QTemporaryDir base;
        QVERIFY(base.isValid());
        const QString real = base.filePath(QStringLiteral("real"));
        QVERIFY(QDir().mkpath(real));
        const QString link = base.filePath(QStringLiteral("link"));
        if (!QFile::link(real, link) || QFileInfo(link).canonicalFilePath() != QFileInfo(real).canonicalFilePath()) {
            QSKIP("Filesystem did not create a followed symlink");
        }

        QCOMPARE(TMedia::resolvedMediaPath(link + QStringLiteral("/gone.wav")), QFileInfo(real).canonicalFilePath() + QStringLiteral("/gone.wav"));
    }

    // ".." after a symlink leads out of the folder the link points to, not back out of the link
    void resolve_dotDotAfterSymlink_followsTheLink()
    {
        QTemporaryDir base;
        QVERIFY(base.isValid());
        const QString sub = base.filePath(QStringLiteral("other/sub"));
        QVERIFY(QDir().mkpath(sub));
        QVERIFY(QDir().mkpath(base.filePath(QStringLiteral("pack"))));
        QFile tone(base.filePath(QStringLiteral("other/tone.wav")));
        QVERIFY(tone.open(QIODevice::WriteOnly));
        tone.close();
        const QString link = base.filePath(QStringLiteral("pack/link"));
        if (!QFile::link(sub, link) || QFileInfo(link).canonicalFilePath() != QFileInfo(sub).canonicalFilePath()) {
            QSKIP("Filesystem did not create a followed symlink");
        }
        const QString other = QFileInfo(base.filePath(QStringLiteral("other"))).canonicalFilePath();

        QCOMPARE(TMedia::resolvedMediaPath(link + QStringLiteral("/../tone.wav")), other + QStringLiteral("/tone.wav"));
        QCOMPARE(TMedia::resolvedMediaPath(link + QStringLiteral("/../gone.wav")), other + QStringLiteral("/gone.wav"));
    }

    void cache_sameNameInTwoFolders_twoCopies()
    {
        QCOMPARE(cacheName(profileHome() + QStringLiteral("/sounds/steps/sand/3.ogg")), QStringLiteral("_profile/sounds/steps/sand/3.ogg"));
        QCOMPARE(cacheName(profileHome() + QStringLiteral("/sounds/combat/sword/3.ogg")), QStringLiteral("_profile/sounds/combat/sword/3.ogg"));
    }

    void cache_fileAlreadyInMedia_playedWhereItIs() { QCOMPARE(cacheName(mediaRoot() + QStringLiteral("/weather/rain.wav")), QStringLiteral("weather/rain.wav")); }

    void cache_fileOutsideTheProfile_mirroredUnderItsOwnFolder() { QCOMPARE(cacheName(QStringLiteral("/home/user/packs/steps/3.ogg")), QStringLiteral("_absolute/home/user/packs/steps/3.ogg")); }

    // A profile whose name starts with this one's is not inside it
    void cache_siblingProfile_notTakenForThisOne()
    {
        QCOMPARE(cacheName(profileHome() + QStringLiteral("2/sounds/3.ogg")), QStringLiteral("_absolute/home/user/.config/mudlet/profiles/Default2/sounds/3.ogg"));
    }

    void cache_dotDotSegments_resolvedFirst()
    {
        QCOMPARE(cacheName(profileHome() + QStringLiteral("/sounds/../music/a.ogg")), QStringLiteral("_profile/music/a.ogg"));
        QCOMPARE(cacheName(profileHome() + QStringLiteral("/../Other/a.ogg")), QStringLiteral("_absolute/home/user/.config/mudlet/profiles/Other/a.ogg"));
    }

#if defined(Q_OS_WINDOWS)
    void cache_driveLetter_becomesAFolder()
    {
        const QString home = QStringLiteral("C:/Users/me/mudlet/profiles/Default");
        QCOMPARE(TMedia::mediaCacheFileName(home, home + QStringLiteral("/media"), QStringLiteral("D:/packs/3.ogg")), QStringLiteral("_absolute/D/packs/3.ogg"));
        QCOMPARE(TMedia::mediaCacheFileName(home, home + QStringLiteral("/media"), QStringLiteral("d:/packs/3.ogg")), QStringLiteral("_absolute/D/packs/3.ogg"));
    }

    // A server named D is not drive D
    void cache_networkShare_apartFromDrives()
    {
        const QString home = QStringLiteral("C:/Users/me/mudlet/profiles/Default");
        QCOMPARE(TMedia::mediaCacheFileName(home, home + QStringLiteral("/media"), QStringLiteral("//D/packs/3.ogg")), QStringLiteral("_absolute/UNC/D/packs/3.ogg"));
    }
#endif

    void reserved_copyFolders_refusedInAnyCase()
    {
        QVERIFY(TMedia::mediaFileNameReservedForCopies(QStringLiteral("_profile/sounds/3.ogg")));
        QVERIFY(TMedia::mediaFileNameReservedForCopies(QStringLiteral("_absolute/home/3.ogg")));
        QVERIFY(TMedia::mediaFileNameReservedForCopies(QStringLiteral("./_PROFILE/3.ogg")));
        QVERIFY(TMedia::mediaFileNameReservedForCopies(QStringLiteral("_profile")));
    }

    void reserved_otherFolders_allowed()
    {
        QVERIFY(!TMedia::mediaFileNameReservedForCopies(QStringLiteral("sounds/_profile/3.ogg")));
        QVERIFY(!TMedia::mediaFileNameReservedForCopies(QStringLiteral("_profiles/3.ogg")));
        QVERIFY(!TMedia::mediaFileNameReservedForCopies(QStringLiteral("3.ogg")));
    }
};

QTEST_GUILESS_MAIN(TMediaPathTraversalTest)

#include "TMediaPathTraversalTest.moc"
