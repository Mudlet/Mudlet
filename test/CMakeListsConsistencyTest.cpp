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

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>

/*
 * Build-file consistency check for src/CMakeLists.txt.
 *
 * The project lists its sources and headers explicitly (rather than globbing),
 * which is the recommended CMake practice but makes it easy to forget to add a
 * new file - or to paste one in twice. That is exactly what happened in #9344,
 * where TEncodingHelper.h (and, it turned out, several other headers) were left
 * out of mudlet_HDRS, and a few sources were listed twice.
 *
 * This test walks the top-level files in src/ and the text of src/CMakeLists.txt
 * and fails if any .cpp / .h is missing from the build file, or is listed more
 * than once. The src/ path is provided at configure time via MUDLET_SRC_DIR.
 *
 * Scope note: only top-level .cpp and .h files directly in src/ are checked.
 * Files in subdirectories (updater/, etc.) are listed with a path prefix and are
 * left for a future extension if needed.
 */
class CMakeListsConsistencyTest : public QObject
{
    Q_OBJECT

    static QString srcDir() { return QStringLiteral(MUDLET_SRC_DIR); }

    static QString readCMakeLists()
    {
        QFile file(srcDir() + QStringLiteral("/CMakeLists.txt"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        // Drop comments so a filename mentioned in a comment cannot be mistaken
        // for a build-list entry.
        QString content;
        const QString raw = QString::fromUtf8(file.readAll());
        const auto lines = raw.split(QLatin1Char('\n'));
        for (const QString& line : lines) {
            const qsizetype hash = line.indexOf(QLatin1Char('#'));
            content.append(hash >= 0 ? line.left(hash) : line);
            content.append(QLatin1Char('\n'));
        }
        return content;
    }

    // All *.cpp / *.h base names referenced anywhere in the (comment-stripped)
    // CMakeLists text. Used for the "is it listed at all?" check, so a file added
    // only through a conditional list(APPEND ...) still counts as listed.
    static QSet<QString> referencedNames(const QString& content)
    {
        QSet<QString> names;
        static const QRegularExpression re(QStringLiteral("([A-Za-z0-9_]+\\.(?:cpp|h))\\b"));
        auto it = re.globalMatch(content);
        while (it.hasNext()) {
            names.insert(it.next().captured(1));
        }
        return names;
    }

    // Counts only "bare" list entries - a filename alone on its line, the form
    // used inside the set(mudlet_SRCS ...) / set(mudlet_HDRS ...) lists. This is
    // where accidental duplicates appear. Conditional list(APPEND ...) lines and
    // OTHER_FILES references are deliberately excluded, since the same file may be
    // appended under several mutually-exclusive platform branches.
    static QMap<QString, int> bareEntryCounts(const QString& content)
    {
        QMap<QString, int> counts;
        static const QRegularExpression re(QStringLiteral("^[ \\t]*([A-Za-z0-9_./]+\\.(?:cpp|h))[ \\t]*$"));
        const auto lines = content.split(QLatin1Char('\n'));
        for (const QString& line : lines) {
            const auto match = re.match(line);
            if (!match.hasMatch()) {
                continue;
            }
            QString name = match.captured(1);
            const qsizetype slash = name.lastIndexOf(QLatin1Char('/'));
            if (slash >= 0) {
                name = name.mid(slash + 1);
            }
            counts[name] += 1;
        }
        return counts;
    }

    static QStringList topLevelSources()
    {
        QDir dir(srcDir());
        return dir.entryList({QStringLiteral("*.cpp"), QStringLiteral("*.h")}, QDir::Files);
    }

private slots:

    void sourceDirectory_isReadable()
    {
        QVERIFY2(QDir(srcDir()).exists(), qPrintable(QStringLiteral("src dir not found: %1").arg(srcDir())));
        QVERIFY2(!readCMakeLists().isEmpty(), "src/CMakeLists.txt could not be read");
        QVERIFY2(!topLevelSources().isEmpty(), "no source files found in src/");
    }

    void everySourceAndHeader_isListed()
    {
        const QSet<QString> refs = referencedNames(readCMakeLists());

        QStringList missing;
        for (const QString& file : topLevelSources()) {
            if (!refs.contains(file)) {
                missing << file;
            }
        }
        missing.sort();

        QVERIFY2(missing.isEmpty(), qPrintable(QStringLiteral("Files present in src/ but not listed in src/CMakeLists.txt: %1").arg(missing.join(QStringLiteral(", ")))));
    }

    void noSourceOrHeader_isListedTwice()
    {
        const QMap<QString, int> refs = bareEntryCounts(readCMakeLists());
        const QStringList files = topLevelSources();
        const QSet<QString> present(files.cbegin(), files.cend());

        QStringList duplicates;
        for (auto it = refs.cbegin(); it != refs.cend(); ++it) {
            if (it.value() > 1 && present.contains(it.key())) {
                duplicates << QStringLiteral("%1 (x%2)").arg(it.key()).arg(it.value());
            }
        }
        duplicates.sort();

        QVERIFY2(duplicates.isEmpty(), qPrintable(QStringLiteral("Files listed more than once in src/CMakeLists.txt: %1").arg(duplicates.join(QStringLiteral(", ")))));
    }
};

QTEST_GUILESS_MAIN(CMakeListsConsistencyTest)

#include "CMakeListsConsistencyTest.moc"
