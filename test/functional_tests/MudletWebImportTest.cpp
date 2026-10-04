/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

// The archives read here are what Mudlet Web's "Export profiles…" writes
// (mudletProfileExport.ts in Mudlet/mudlet-web): one folder per profile with a
// save in current/, desktop's connection files beside it - or, from older
// exports, only Mudlet Web's own .mudlet/connection.json - and modules listed
// by their place inside the profile. The import is a dialog button with no Lua
// entry point, hence a functional test.

#include <QtTest/QtTest>

#include <QTemporaryDir>
#include <zip.h>

#include "MudletWebImport.h"

#include "GroupedTest.h"

class MudletWebImportTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mRoot;

    QString profilesPath() const { return mRoot.filePath(qsl("profiles")); }
    QString archivePath() const { return mRoot.filePath(qsl("export.zip")); }

    // Every entry is dated alike, as Mudlet Web dates an export, unless given a time of its own
    static bool writeArchive(const QString& path, const QList<QPair<QString, QByteArray>>& entries, const QHash<QString, QDateTime>& modified = {})
    {
        const QDateTime exported(QDate(2026, 10, 4), QTime(12, 0));
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!archive) {
            return false;
        }
        for (const auto& [name, contents] : entries) {
            zip_source* source = zip_source_buffer(archive, contents.constData(), contents.size(), 0);
            const zip_int64_t index = source ? zip_file_add(archive, name.toUtf8().constData(), source, ZIP_FL_ENC_UTF_8) : -1;
            // Stored, so that damage() can find an entry's bytes
            if (index < 0 || zip_set_file_compression(archive, static_cast<zip_uint64_t>(index), ZIP_CM_STORE, 0) != 0
                || zip_file_set_mtime(archive, static_cast<zip_uint64_t>(index), static_cast<time_t>(modified.value(name, exported).toSecsSinceEpoch()), 0) != 0) {
                zip_source_free(source);
                zip_discard(archive);
                return false;
            }
        }
        return zip_close(archive) == 0;
    }

    // Changes an entry's data but not its checksum, so reading it fails partway
    static bool damage(const QString& path, const QByteArray& marker)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadWrite)) {
            return false;
        }
        QByteArray bytes = file.readAll();
        const qsizetype at = bytes.indexOf(marker);
        if (at < 0) {
            return false;
        }
        bytes[at] = bytes.at(at) == 'X' ? 'Y' : 'X';
        return file.seek(0) && file.write(bytes) == bytes.size();
    }

    static QByteArray profileItem(const QString& text)
    {
        QByteArray bytes;
        QDataStream stream(&bytes, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_5_12);
        stream << text;
        return bytes;
    }

    static QString readProfileItem(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return qsl("<missing>");
        }
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_5_12);
        QString text;
        stream >> text;
        return text;
    }

    static QByteArray save(const QString& marker)
    {
        return qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE MudletPackage>\n"
                   "<MudletPackage version=\"1.001\"><HostPackage><Host><name>%1</name></Host></HostPackage></MudletPackage>\n")
                .arg(marker)
                .toUtf8();
    }

    static QByteArray json(const QJsonObject& object) { return QJsonDocument(object).toJson(); }

    static QString newestSave(const QString& home)
    {
        const QStringList saves = QDir(home + qsl("/current")).entryList({qsl("*.xml")}, QDir::Files, QDir::Time);
        return saves.isEmpty() ? QString() : saves.constFirst();
    }

private slots:
    void init()
    {
        QVERIFY(mRoot.isValid());
        QDir(profilesPath()).removeRecursively();
        QVERIFY(QDir().mkpath(profilesPath()));
    }

    void test_eachProfileFolderBecomesAProfileOfItsOwn()
    {
        QVERIFY(writeArchive(archivePath(),
                             {{qsl("Arkadia/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))},
                              {qsl("Arkadia/url"), profileItem(qsl("arkadia.rpg.pl"))},
                              {qsl("Arkadia/scripts/combat.xml"), save(qsl("module"))},
                              {qsl("Arkadia/logs/Arkadia 2026-10-04.html"), QByteArray("<html/>")},
                              {qsl("Arkadia/.mudlet/connection.json"), json({{qsl("mode"), qsl("mud")}, {qsl("host"), qsl("stale.example")}})},
                              {qsl("Aardwolf/current/2026-10-04#12-00-00.xml"), save(qsl("aard"))}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        QCOMPARE(result.profiles, (QStringList{qsl("Aardwolf"), qsl("Arkadia")}));
        const QString home = profilesPath() + qsl("/Arkadia");
        QVERIFY(QFileInfo::exists(home + qsl("/scripts/combat.xml")));
        QVERIFY(QFileInfo::exists(home + qsl("/log/Arkadia 2026-10-04.html")));
        QVERIFY(!QFileInfo::exists(home + qsl("/.mudlet")));
        // the native file wins over the sidecar, which is only for exports that lack them
        QCOMPARE(readProfileItem(home + qsl("/url")), qsl("arkadia.rpg.pl"));
        QVERIFY(QFileInfo::exists(profilesPath() + qsl("/Aardwolf/current/2026-10-04#12-00-00.xml")));
        // nothing is left behind beside the profiles folder
        QCOMPARE(QDir(mRoot.path()).entryList({qsl("mudlet-web-import-*")}, QDir::Dirs | QDir::Hidden), QStringList());
    }

    void test_aFreshInstallWithoutAProfilesFolderGetsOne()
    {
        QVERIFY(QDir(profilesPath()).removeRecursively());
        QVERIFY(writeArchive(archivePath(), {{qsl("First/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        QCOMPARE(result.profiles, QStringList{qsl("First")});
        QVERIFY(QFileInfo::exists(profilesPath() + qsl("/First/current/2026-10-04#12-00-00.xml")));
    }

    void test_aNameThatIsTakenGetsANumber()
    {
        QVERIFY(QDir().mkpath(profilesPath() + qsl("/arkadia")));
        QVERIFY(writeArchive(archivePath(), {{qsl("Arkadia/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("Arkadia (2)")});
        QVERIFY(QDir(profilesPath() + qsl("/arkadia")).isEmpty());
    }

    void test_theTimeStampedSaveIsTheOneDesktopLoads()
    {
        // a hand-named save sorts after a stamp by name, and must still not be taken for the newest
        QVERIFY(writeArchive(archivePath(),
                             {{qsl("P/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))},
                              {qsl("P/current/before-raid.xml"), save(qsl("named"))},
                              {qsl("P/current/2025-01-01#00-00-00.xml"), save(qsl("old"))},
                              {qsl("P/map/2025-01-01#00-00-00map.dat"), QByteArray("old")},
                              {qsl("P/map/2026-10-04#12-00-00map.dat"), QByteArray("fresh")}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("P")});
        QCOMPARE(newestSave(profilesPath() + qsl("/P")), qsl("2026-10-04#12-00-00.xml"));
        const QStringList maps = QDir(profilesPath() + qsl("/P/map")).entryList(QDir::Files, QDir::Time);
        QCOMPARE(maps.constFirst(), qsl("2026-10-04#12-00-00map.dat"));
    }

    void test_aSaveTheArchiveDatesLaterIsTheOneDesktopLoads()
    {
        // desktop's autosave, from a profile folder zipped by hand, can be newer than its last stamped save
        QVERIFY(writeArchive(archivePath(),
                             {{qsl("current/2026-10-04#12-00-00.xml"), save(qsl("stamped"))},
                              {qsl("current/autosave.xml"), save(qsl("autosave"))},
                              {qsl("map/2026-10-04#12-00-00map.dat"), QByteArray("stamped")},
                              {qsl("map/autosave.dat"), QByteArray("autosave")}},
                             {{qsl("current/autosave.xml"), QDateTime(QDate(2026, 10, 4), QTime(12, 30))}, {qsl("map/autosave.dat"), QDateTime(QDate(2026, 10, 4), QTime(12, 30))}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("export")});
        QCOMPARE(newestSave(profilesPath() + qsl("/export")), qsl("autosave.xml"));
        QCOMPARE(QDir(profilesPath() + qsl("/export/map")).entryList(QDir::Files, QDir::Time).constFirst(), qsl("autosave.dat"));
    }

    void test_aNameWithAPercentSignKeepsIt()
    {
        QVERIFY(QDir().mkpath(profilesPath() + qsl("/50%2 off")));
        QVERIFY(writeArchive(archivePath(), {{qsl("50%2 off/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("50%2 off (2)")});
    }

    void test_anOlderExportTakesItsConnectionFromTheSidecar()
    {
        QVERIFY(writeArchive(archivePath(),
                             {{qsl("Old/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))},
                              {qsl("Old/.mudix/connection.json"),
                               json({{qsl("mode"), qsl("mud")},
                                     {qsl("host"), qsl("mud.example")},
                                     {qsl("port"), 4000},
                                     {qsl("tls"), true},
                                     {qsl("autoReconnect"), false},
                                     {qsl("reconnectOnDrop"), true},
                                     {qsl("charLoginAccount"), qsl("Zoë")},
                                     {qsl("description"), qsl("main")}})}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("Old")});
        QVERIFY2(result.warnings.isEmpty(), qPrintable(result.warnings.join(qsl("; "))));
        const QString home = profilesPath() + qsl("/Old");
        QCOMPARE(readProfileItem(home + qsl("/url")), qsl("mud.example"));
        QCOMPARE(readProfileItem(home + qsl("/port")), qsl("4000"));
        QCOMPARE(readProfileItem(home + qsl("/ssl_tsl")), QString::number(Qt::Checked));
        QCOMPARE(readProfileItem(home + qsl("/autologin")), QString::number(Qt::Unchecked));
        QCOMPARE(readProfileItem(home + qsl("/autoreconnect")), QString::number(Qt::Checked));
        QCOMPARE(readProfileItem(home + qsl("/login")), qsl("Zoë"));
        QCOMPARE(readProfileItem(home + qsl("/description")), qsl("main"));
    }

    void test_aWebSocketProfileSaysWhatItNeeds()
    {
        QVERIFY(writeArchive(archivePath(),
                             {{qsl("Web/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))},
                              {qsl("Web/.mudlet/connection.json"), json({{qsl("mode"), qsl("websocket")}, {qsl("url"), qsl("wss://example.org/ws")}, {qsl("host"), qsl("ignored")}})}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("Web")});
        QCOMPARE(result.warnings.filter(qsl("WebSocket")).size(), 1);
        QVERIFY(!QFileInfo::exists(profilesPath() + qsl("/Web/url")));
    }

    void test_aConnectionMadeBeforeModesIsAWebSocketOne()
    {
        QVERIFY(writeArchive(archivePath(),
                             {{qsl("Web/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))},
                              {qsl("Web/url"), profileItem(QString())},
                              {qsl("Web/.mudlet/connection.json"), json({{qsl("url"), qsl("wss://example.org/ws")}})}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("Web")});
        QCOMPARE(result.warnings.filter(qsl("WebSocket")).size(), 1);
    }

    void test_aProfileWithoutAnAddressSaysSo()
    {
        QVERIFY(writeArchive(archivePath(),
                             {{qsl("Bare/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))}, {qsl("Bare/.mudlet/connection.json"), json({{qsl("mode"), qsl("mud")}, {qsl("port"), 23}})}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("Bare")});
        QCOMPARE(result.warnings.filter(qsl("no server address")).size(), 1);
    }

    void test_aSaveThatWouldNotUnpackIsNotAdded()
    {
        QVERIFY(writeArchive(archivePath(),
                             {{qsl("Broken/current/2026-10-04#12-00-00.xml"), save(qsl("damaged-save"))},
                              {qsl("Broken/scripts/kept.lua"), QByteArray("fine")},
                              {qsl("Fine/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))}}));
        QVERIFY(damage(archivePath(), "damaged-save"));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("Fine")});
        QCOMPARE(result.warnings.filter(qsl("\"Broken\" was not added")).size(), 1);
        QVERIFY(!QFileInfo::exists(profilesPath() + qsl("/Broken")));
    }

    void test_aSaveCutShortIsNotTheOneDesktopLoads()
    {
        QVERIFY(writeArchive(archivePath(), {{qsl("P/current/2026-10-04#12-00-00.xml"), save(qsl("damaged-save"))}, {qsl("P/current/2026-10-03#12-00-00.xml"), save(qsl("whole"))}}));
        QVERIFY(damage(archivePath(), "damaged-save"));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("P")});
        QCOMPARE(result.warnings.filter(qsl("could not be written")).size(), 1);
        QCOMPARE(QDir(profilesPath() + qsl("/P/current")).entryList(QDir::Files), QStringList{qsl("2026-10-03#12-00-00.xml")});
    }

    void test_nothingLandsOutsideTheProfile()
    {
        QVERIFY(writeArchive(archivePath(),
                             {{qsl("P/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))},
                              {qsl("P/../../escaped.txt"), QByteArray("no")},
                              {qsl("/abs.txt"), QByteArray("no")},
                              {qsl("P/scripts/kept.lua"), QByteArray("yes")}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("P")});
        QCOMPARE(result.warnings.filter(qsl("outside")).size(), 1);
        QVERIFY(QFileInfo::exists(profilesPath() + qsl("/P/scripts/kept.lua")));
        QVERIFY(!QFileInfo::exists(mRoot.filePath(qsl("escaped.txt"))));
        QVERIFY(!QFileInfo::exists(QFileInfo(mRoot.path()).absolutePath() + qsl("/escaped.txt")));
    }

    void test_aNameThatCannotBeAFolderIsMadeOne()
    {
        QVERIFY(writeArchive(archivePath(), {{qsl("Bad..Name./current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("Bad_Name")});
    }

    void test_anArchiveWithoutAProfileIsRefused()
    {
        QVERIFY(writeArchive(archivePath(), {{qsl("notes/readme.txt"), QByteArray("hi")}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QVERIFY(result.profiles.isEmpty());
        QVERIFY(!result.error.isEmpty());
        QVERIFY(QDir(profilesPath()).isEmpty());
    }

    void test_aFileThatIsNotAnArchiveIsRefused()
    {
        QFile file(archivePath());
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("not a zip");
        file.close();

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QVERIFY(result.profiles.isEmpty());
        QVERIFY(result.error.contains(qsl("export.zip")));
    }

    void test_aModuleIsFoundInsideTheProfile()
    {
        const QString home = profilesPath() + qsl("/P");
        QVERIFY(QDir().mkpath(home + qsl("/scripts")));
        QVERIFY(QDir().mkpath(home + qsl("/Combat")));
        for (const auto& path : {qsl("/scripts/combat.xml"), qsl("/Combat/Combat.mpackage")}) {
            QFile file(home + path);
            QVERIFY(file.open(QIODevice::WriteOnly));
        }

        // Mudlet Web lists it by its place in the profile
        QCOMPARE(MudletWebImport::locateModuleFile(home, qsl("Scripts"), qsl("scripts/combat.xml")), home + qsl("/scripts/combat.xml"));
        // another computer's path, gone here, but the export brought the file along
        QCOMPARE(MudletWebImport::locateModuleFile(home, qsl("Combat"), qsl("/home/someone/mods/Combat.mpackage")), home + qsl("/Combat/Combat.mpackage"));
        QCOMPARE(MudletWebImport::locateModuleFile(home, qsl("Combat"), qsl("C:\\Users\\someone\\Combat.mpackage")), home + qsl("/Combat/Combat.mpackage"));
        // a module that is where the save says is left alone, as is one found nowhere
        QCOMPARE(MudletWebImport::locateModuleFile(home, qsl("Other"), home + qsl("/scripts/combat.xml")), home + qsl("/scripts/combat.xml"));
        QCOMPARE(MudletWebImport::locateModuleFile(home, qsl("Gone"), qsl("/nowhere/Gone.xml")), qsl("/nowhere/Gone.xml"));
    }
};

MUDLET_GROUPED_TEST_MAIN(MudletWebImportTest)
#include "MudletWebImportTest.moc"
