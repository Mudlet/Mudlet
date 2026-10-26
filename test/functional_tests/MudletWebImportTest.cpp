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

#include <QMessageBox>
#include <QTemporaryDir>
#include <chrono>
#include <memory>
#include <optional>
#include <zip.h>

#include "CredentialManager.h"
#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "MudletWebImport.h"
#include "PortableModeTestHelper.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class MudletWebImportTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mRoot;
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

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

    // One deflated entry whose headers claim it unpacks to statedSize bytes
    static bool writeUnderstatedArchive(const QString& path, const QString& savePath, const QString& name, const QByteArray& contents, quint32 statedSize)
    {
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!archive) {
            return false;
        }
        const QByteArray saveBytes = save(qsl("fresh"));
        zip_source* saveSource = zip_source_buffer(archive, saveBytes.constData(), saveBytes.size(), 0);
        zip_source* source = zip_source_buffer(archive, contents.constData(), contents.size(), 0);
        if (zip_file_add(archive, savePath.toUtf8().constData(), saveSource, ZIP_FL_ENC_UTF_8) < 0 || zip_file_add(archive, name.toUtf8().constData(), source, ZIP_FL_ENC_UTF_8) < 0
            || zip_close(archive) != 0) {
            zip_discard(archive);
            return false;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadWrite)) {
            return false;
        }
        QByteArray bytes = file.readAll();
        const QByteArray nameBytes = name.toUtf8();
        const QByteArray size(reinterpret_cast<const char*>(&statedSize), sizeof(statedSize));
        int patched = 0;
        for (qsizetype at = bytes.indexOf(nameBytes); at >= 0; at = bytes.indexOf(nameBytes, at + 1)) {
            if (at >= 30 && bytes.mid(at - 30, 4) == QByteArray("PK\x03\x04", 4)) {
                bytes.replace(at - 30 + 22, 4, size);
                ++patched;
            } else if (at >= 46 && bytes.mid(at - 46, 4) == QByteArray("PK\x01\x02", 4)) {
                bytes.replace(at - 46 + 24, 4, size);
                ++patched;
            }
        }
        return patched == 2 && file.seek(0) && file.write(bytes) == bytes.size();
    }

    // Everything this process has handed to write() so far
    static qint64 bytesWritten()
    {
        QFile io(qsl("/proc/self/io"));
        if (!io.open(QIODevice::ReadOnly)) {
            return -1;
        }
        for (const QByteArray& line : io.readAll().split('\n')) {
            if (line.startsWith("wchar:")) {
                return line.mid(6).trimmed().toLongLong();
            }
        }
        return -1;
    }

    // Through CredentialManager's own choice of store: the keychain, unless MUDLET_TEST_MODE puts it in a file
    static bool storePassword(const QString& profile, const QString& key, const QString& password)
    {
        auto stored = std::make_shared<std::optional<bool>>();
        CredentialManager manager;
        manager.storePassword(profile, key, password, [stored](bool success, const QString&) {
            *stored = success;
        });
        return QTest::qWaitFor(
                       [stored]() {
                           return stored->has_value();
                       },
                       15s)
               && stored->value();
    }

    static std::optional<QString> retrievePassword(const QString& profile, const QString& key)
    {
        auto answer = std::make_shared<std::optional<std::pair<bool, QString>>>();
        CredentialManager manager;
        manager.retrievePassword(profile, key, [answer](bool success, const QString& password, const QString&) {
            *answer = std::make_pair(success, password);
        });
        if (!QTest::qWaitFor(
                    [answer]() {
                        return answer->has_value();
                    },
                    15s)
            || !answer->value().first) {
            return std::nullopt;
        }
        return answer->value().second;
    }

    bool startMudlet()
    {
        if (mudlet::self()) {
            return true;
        }
        if (!mConfigDir.isValid() || !QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path()))) {
            return false;
        }
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(true);
        mudlet::self()->startAutoLogin({});
        return MudletApp::getMudletPath(enums::profilesPath).startsWith(mConfigDir.path())
               && QTest::qWaitFor(
                       []() {
                           return mudlet::self()->mpConnectionDialog && mudlet::self()->mpConnectionDialog->isVisible();
                       },
                       5s);
    }

private slots:
    void cleanupTestCase()
    {
        if (mudlet::self()) {
            delete mudlet::self();
            mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        }
    }

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
        QCOMPARE(Host::locateModuleFile(home, qsl("Scripts"), qsl("scripts/combat.xml")), home + qsl("/scripts/combat.xml"));
        // another computer's path, gone here, but the export brought the file along
        QCOMPARE(Host::locateModuleFile(home, qsl("Combat"), qsl("/home/someone/mods/Combat.mpackage")), home + qsl("/Combat/Combat.mpackage"));
        QCOMPARE(Host::locateModuleFile(home, qsl("Combat"), qsl("C:\\Users\\someone\\Combat.mpackage")), home + qsl("/Combat/Combat.mpackage"));
        // a module that is where the save says is left alone, as is one found nowhere
        QCOMPARE(Host::locateModuleFile(home, qsl("Other"), home + qsl("/scripts/combat.xml")), home + qsl("/scripts/combat.xml"));
        QCOMPARE(Host::locateModuleFile(home, qsl("Gone"), qsl("/nowhere/Gone.xml")), qsl("/nowhere/Gone.xml"));
    }

    // The export keeps a module that lives inside the profile at its place there, while
    // the save names it by where the other computer kept the profile - under its old name
    void test_aModuleKeptInsideTheProfileIsFoundOnAnotherComputer()
    {
        QVERIFY(QDir().mkpath(profilesPath() + qsl("/arkadia")));
        QVERIFY(writeArchive(archivePath(), {{qsl("Arkadia/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))}, {qsl("Arkadia/scripts/combat.xml"), save(qsl("module"))}}));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("Arkadia (2)")});
        const QString home = profilesPath() + qsl("/Arkadia (2)");
        QVERIFY(QFileInfo::exists(home + qsl("/scripts/combat.xml")));
        QCOMPARE(Host::locateModuleFile(home, qsl("combat"), qsl("/home/someone/.config/mudlet/profiles/Arkadia/scripts/combat.xml")), home + qsl("/scripts/combat.xml"));
        QCOMPARE(Host::locateModuleFile(home, qsl("combat"), qsl("C:\\Users\\someone\\.config\\mudlet\\profiles\\Arkadia\\scripts\\combat.xml")), home + qsl("/scripts/combat.xml"));
    }

    // "Foo:" and "Foo_" are both "Foo_" as folder names
    void test_aProfileThatFailsLeavesNothingToTheNextOfTheSameName()
    {
        QVERIFY(writeArchive(archivePath(),
                             {{qsl("Foo:/current/2026-10-04#12-00-00.xml"), save(qsl("damaged-save"))},
                              {qsl("Foo:/scripts/stray.lua"), QByteArray("from the other profile")},
                              {qsl("Foo_/current/2026-10-04#12-00-00.xml"), save(qsl("fresh"))}}));
        QVERIFY(damage(archivePath(), "damaged-save"));

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());

        QCOMPARE(result.profiles, QStringList{qsl("Foo_")});
        QCOMPARE(result.warnings.filter(qsl("\"Foo_\" was not added")).size(), 1);
        QVERIFY2(!QFileInfo::exists(profilesPath() + qsl("/Foo_/scripts/stray.lua")), "the profile that failed left its files in the one that went in");
    }

    // The stated size is what an archive is judged by, so data running past it is not written
    void test_anEntryLargerThanItSaysIsCutOffThere()
    {
        const qint64 before = bytesWritten();
        if (before < 0) {
            QSKIP("needs /proc/self/io to count what is written");
        }
        const QByteArray zeros(64 * 1024 * 1024, '\0');
        QVERIFY(writeUnderstatedArchive(archivePath(), qsl("P/current/2026-10-04#12-00-00.xml"), qsl("P/sounds/understated.wav"), zeros, 1000));
        QVERIFY(QFileInfo(archivePath()).size() < 1024 * 1024);

        const qint64 start = bytesWritten();
        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath());
        const qint64 written = bytesWritten() - start;

        QCOMPARE(result.profiles, QStringList{qsl("P")});
        QVERIFY(!QFileInfo::exists(profilesPath() + qsl("/P/sounds/understated.wav")));
        QVERIFY2(written < 4 * 1024 * 1024, qPrintable(qsl("%1 bytes were written for an archive that says it holds about 1 KB").arg(written)));
    }

    void test_anArchiveThatWouldNotFitIsRefusedBeforeAnythingIsWritten()
    {
        const QByteArray zeros(2 * 1024 * 1024, '\0');
        QVERIFY(writeArchive(archivePath(), {{qsl("P/current/2026-10-04#12-00-00.xml"), save(qsl("fits"))}, {qsl("P/sounds/big.wav"), zeros}}));

        const auto refused = MudletWebImport::importArchive(archivePath(), profilesPath(), 1024 * 1024);
        QVERIFY2(!refused.error.isEmpty(), "an archive that unpacks to 2 MB went into 1 MB");
        QVERIFY(refused.profiles.isEmpty());
        QVERIFY(QDir(profilesPath()).isEmpty());
        QVERIFY(QDir(mRoot.path()).entryList({qsl("mudlet-web-import-*")}, QDir::Dirs | QDir::Hidden).isEmpty());

        const auto result = MudletWebImport::importArchive(archivePath(), profilesPath(), 4 * 1024 * 1024);
        QCOMPARE(result.profiles, QStringList{qsl("P")});
        QCOMPARE(QFileInfo(profilesPath() + qsl("/P/sounds/big.wav")).size(), zeros.size());
    }

    // A profile folder deleted outside Mudlet leaves its stored passwords behind, under
    // the name an import may then give a profile for another game
    void test_anImportedProfileDoesNotInheritAForgottenPassword()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(startMudlet());
        auto* dialog = mudlet::self()->mpConnectionDialog.data();
        const QString name = qsl("Imported over a forgotten one");
        QVERIFY(!QFileInfo::exists(MudletApp::getMudletPath(enums::profileHomePath, name)));
        QVERIFY(storePassword(name, qsl("character"), qsl("forgotten-secret")));
        const auto forgotten = retrievePassword(name, qsl("character"));
        QVERIFY2(forgotten.has_value(), "the forgotten password could not be planted");
        QCOMPARE(*forgotten, qsl("forgotten-secret"));

        QVERIFY(writeArchive(archivePath(), {{qsl("%1/current/2026-10-04#12-00-00.xml").arg(name), save(qsl("fresh"))}, {qsl("%1/url").arg(name), profileItem(qsl("game.example"))}}));
        QTimer answer;
        connect(&answer, &QTimer::timeout, this, []() {
            if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
                box->done(QMessageBox::Ok);
            }
        });
        answer.start(20ms);
        dialog->importProfilesFrom(archivePath());
        QVERIFY(QTest::qWaitFor(
                [dialog, name]() {
                    return dialog->listWidget_profiles->currentItem() && dialog->listWidget_profiles->currentItem()->data(dlgConnectionProfiles::csmNameRole).toString() == name
                           && dialog->import_profile_button->isEnabled();
                },
                15s));
        const bool filled = QTest::qWaitFor(
                [dialog]() {
                    return !dialog->character_password_entry->text().isEmpty();
                },
                2s);
        QVERIFY2(!filled, qPrintable(qsl("the imported profile shows the password \"%1\"").arg(dialog->character_password_entry->text())));
        QVERIFY2(!retrievePassword(name, qsl("character")).has_value(), "the forgotten password is still stored under the imported profile's name");

        // a password of its own still shows, so the empty field above is not a lookup that never ran
        QVERIFY(storePassword(name, qsl("character"), qsl("its-own-secret")));
        QTest::qWait(150ms);
        dialog->slot_itemClicked(dialog->listWidget_profiles->currentItem());
        QTRY_COMPARE_WITH_TIMEOUT(dialog->character_password_entry->text(), qsl("its-own-secret"), 15s);
        answer.stop();
    }
};

MUDLET_GROUPED_TEST_MAIN(MudletWebImportTest)
#include "MudletWebImportTest.moc"
