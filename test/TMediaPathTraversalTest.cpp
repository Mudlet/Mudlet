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
};

QTEST_GUILESS_MAIN(TMediaPathTraversalTest)

#include "TMediaPathTraversalTest.moc"
