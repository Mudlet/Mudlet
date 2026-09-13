/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

/*
 * utils::unzip() is handed whatever a player downloaded: an .mpackage from a
 * repository, a map or theme archive fetched over the network. So it has to
 * answer "no" to a malformed one rather than trust what the archive says about
 * itself, and it has to do it promptly - it runs in unzipAsync()'s worker
 * thread and in the package installer, where nothing can interrupt it.
 *
 * The case that pins that is the third one here. An entry whose central
 * directory declares a compressed size of zero and an uncompressed size of
 * 4 KiB is accepted by libzip - zip_stat_index() fills in the declared 4096 and
 * marks every field valid - but there is no data behind it, so zip_fread()
 * returns 0 immediately and keeps returning 0, because that is how it reports
 * the end of an entry. A loop that runs until it has read the declared number
 * of bytes and only stops early on a negative return therefore never ends.
 * Without the `len <= 0` check in utils::unzip() this case does not fail, it
 * hangs, and ctest's 60s timeout is what reports it.
 *
 * The archives are built byte by byte rather than through libzip because libzip
 * will not write an entry that lies about itself, which is precisely what a
 * hostile or truncated archive does.
 *
 * Run with: ctest -R UnzipBadArchiveTest -V
 */

#include <QtTest/QtTest>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>

#include "utils.h"

#include "GroupedTest.h"

class UnzipBadArchiveTest : public QObject
{
    Q_OBJECT

private:
    // How long utils::unzip() may take over any of these tiny archives. Real
    // work here is microseconds; the number is only large enough that a loaded
    // CI runner cannot trip it, and small enough to be under ctest's timeout so
    // a slow-but-terminating regression reports as this assertion rather than
    // as a killed process.
    static constexpr qint64 scmMaxUnzipMilliseconds = 10000;

    QTemporaryDir mArchiveDir;
    QTemporaryDir mDestinationDir;

    static void appendU16(QByteArray& out, quint16 value)
    {
        out.append(static_cast<char>(value & 0xFFu));
        out.append(static_cast<char>((value >> 8) & 0xFFu));
    }

    static void appendU32(QByteArray& out, quint32 value)
    {
        appendU16(out, static_cast<quint16>(value & 0xFFFFu));
        appendU16(out, static_cast<quint16>((value >> 16) & 0xFFFFu));
    }

    // One stored entry, with the sizes and CRC the headers declare given
    // separately from the bytes actually stored - so an entry can be made to
    // claim more than it carries.
    struct Entry
    {
        QByteArray name;
        QByteArray data;
        quint32 declaredCompressedSize = 0;
        quint32 declaredUncompressedSize = 0;
        quint32 crc = 0;
    };

    static QByteArray buildArchive(const QList<Entry>& entries)
    {
        QByteArray archive;
        QByteArray centralDirectory;
        for (const auto& entry : entries) {
            const quint32 localHeaderOffset = static_cast<quint32>(archive.size());
            appendU32(archive, 0x04034b50u); // local file header signature
            appendU16(archive, 20);          // version needed
            appendU16(archive, 0);           // general purpose flags
            appendU16(archive, 0);           // method: stored
            appendU16(archive, 0);           // modification time
            appendU16(archive, 0);           // modification date
            appendU32(archive, entry.crc);
            appendU32(archive, entry.declaredCompressedSize);
            appendU32(archive, entry.declaredUncompressedSize);
            appendU16(archive, static_cast<quint16>(entry.name.size()));
            appendU16(archive, 0); // extra field length
            archive.append(entry.name);
            archive.append(entry.data);

            appendU32(centralDirectory, 0x02014b50u); // central directory header signature
            appendU16(centralDirectory, 20);          // version made by
            appendU16(centralDirectory, 20);          // version needed
            appendU16(centralDirectory, 0);           // general purpose flags
            appendU16(centralDirectory, 0);           // method: stored
            appendU16(centralDirectory, 0);           // modification time
            appendU16(centralDirectory, 0);           // modification date
            appendU32(centralDirectory, entry.crc);
            appendU32(centralDirectory, entry.declaredCompressedSize);
            appendU32(centralDirectory, entry.declaredUncompressedSize);
            appendU16(centralDirectory, static_cast<quint16>(entry.name.size()));
            appendU16(centralDirectory, 0); // extra field length
            appendU16(centralDirectory, 0); // file comment length
            appendU16(centralDirectory, 0); // disk number start
            appendU16(centralDirectory, 0); // internal attributes
            appendU32(centralDirectory, 0); // external attributes
            appendU32(centralDirectory, localHeaderOffset);
            centralDirectory.append(entry.name);
        }

        const quint32 centralDirectoryOffset = static_cast<quint32>(archive.size());
        archive.append(centralDirectory);
        appendU32(archive, 0x06054b50u); // end of central directory signature
        appendU16(archive, 0);           // this disk
        appendU16(archive, 0);           // disk the central directory starts on
        appendU16(archive, static_cast<quint16>(entries.size()));
        appendU16(archive, static_cast<quint16>(entries.size()));
        appendU32(archive, static_cast<quint32>(centralDirectory.size()));
        appendU32(archive, centralDirectoryOffset);
        appendU16(archive, 0); // archive comment length
        return archive;
    }

    // CRC-32 as the zip format uses it, so a well-formed archive can be built
    // here too rather than only broken ones
    static quint32 zipCrc32(const QByteArray& data)
    {
        quint32 crc = 0xFFFFFFFFu;
        for (const char byte : data) {
            crc ^= static_cast<quint8>(byte);
            for (int bit = 0; bit < 8; ++bit) {
                crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
            }
        }
        return ~crc;
    }

    static Entry storedEntry(const QByteArray& name, const QByteArray& data) { return Entry{name, data, static_cast<quint32>(data.size()), static_cast<quint32>(data.size()), zipCrc32(data)}; }

    QString writeArchive(const QString& fileName, const QByteArray& contents)
    {
        const QString path = mArchiveDir.filePath(fileName);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(contents) != contents.size()) {
            return QString();
        }
        file.close();
        return path;
    }

    // utils::unzip() writes to destination + the entry's name, so the
    // destination it is given has to carry the separator
    QString destination() const { return qsl("%1/").arg(mDestinationDir.path()); }

private slots:
    void initTestCase()
    {
        QVERIFY(mArchiveDir.isValid());
        QVERIFY(mDestinationDir.isValid());
    }

    // The control: the same hand-built archive shape, told the truth about
    // itself, still unpacks - so the refusals below cannot be utils::unzip()
    // simply failing on anything this test writes.
    void test_aWellFormedArchiveIsExtracted()
    {
        const QByteArray contents("the file this archive carries");
        const QString path = writeArchive(qsl("good.zip"), buildArchive({storedEntry("carried.txt", contents)}));
        QVERIFY(!path.isEmpty());

        QElapsedTimer timer;
        timer.start();
        QVERIFY2(utils::unzip(path, destination(), QDir(mDestinationDir.path())), "a well-formed archive was refused");
        QVERIFY2(timer.elapsed() < scmMaxUnzipMilliseconds, "extracting a well-formed archive took far longer than it should have");

        QFile extracted(qsl("%1carried.txt").arg(destination()));
        QVERIFY2(extracted.open(QIODevice::ReadOnly), "the archive's entry was not extracted");
        QCOMPARE(extracted.readAll(), contents);
    }

    void test_aFileThatIsNotAnArchiveIsRefused()
    {
        const QString path = writeArchive(qsl("garbage.zip"), QByteArray("this is not an archive, it is 60-odd bytes of nothing at all"));
        QVERIFY(!path.isEmpty());

        QElapsedTimer timer;
        timer.start();
        QVERIFY2(!utils::unzip(path, destination(), QDir(mDestinationDir.path())), "a file that is not an archive was accepted");
        QVERIFY2(timer.elapsed() < scmMaxUnzipMilliseconds, "refusing a file that is not an archive took far longer than it should have");
    }

    // A download cut off partway through: the central directory it ends with is
    // the part that goes missing, so libzip cannot open it at all.
    void test_aTruncatedArchiveIsRefused()
    {
        QByteArray archive = buildArchive({storedEntry("carried.txt", QByteArray("the file this archive carries"))});
        archive.truncate(archive.size() - 24);
        const QString path = writeArchive(qsl("truncated.zip"), archive);
        QVERIFY(!path.isEmpty());

        QElapsedTimer timer;
        timer.start();
        QVERIFY2(!utils::unzip(path, destination(), QDir(mDestinationDir.path())), "a truncated archive was accepted");
        QVERIFY2(timer.elapsed() < scmMaxUnzipMilliseconds, "refusing a truncated archive took far longer than it should have");
    }

    // The hang: an entry that declares 4 KiB and carries nothing. libzip opens
    // it, reports the declared size as valid, and then returns 0 from every
    // read - so the extraction loop has to treat that as the end of the data
    // and give up, not go round again waiting for bytes that will never come.
    void test_anEntryDeclaringMoreThanItCarriesDoesNotSpin()
    {
        const Entry lying{"lying.txt", QByteArray(), 0, 4096, 0};
        const QString path = writeArchive(qsl("lying.zip"), buildArchive({lying}));
        QVERIFY(!path.isEmpty());

        QElapsedTimer timer;
        timer.start();
        QVERIFY2(!utils::unzip(path, destination(), QDir(mDestinationDir.path())), "an entry declaring more data than it carries was extracted as if it were whole");
        QVERIFY2(timer.elapsed() < scmMaxUnzipMilliseconds, "extraction did not give up on an entry that had no more data to give");
    }
};

#include "UnzipBadArchiveTest.moc"
MUDLET_GROUPED_TEST_MAIN(UnzipBadArchiveTest)
