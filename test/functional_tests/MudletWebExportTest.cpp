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

// The archive "Export to Mudlet Web" writes is read by Mudlet Web's profile
// import (mudletProfileImport.ts in Mudlet/mudlet-web), so what it must hold is
// that importer's contract: the profile folder at the top, one save in
// current/, one map in map/, and each module's file at "<module>/<file name>"
// even when the module lives outside the profile. None of this has a Lua entry
// point - the export is a menu action - hence a functional test.

#include <QtTest/QtTest>

#include <QTemporaryDir>
#include <chrono>
#include <zip.h>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "MudletWebExport.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TMap.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class MudletWebExportTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("MudletWebExport-Test");
    const QString mLocalhost = qsl("localhost");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // Modules live outside the profile on desktop, which is the whole difficulty
    QTemporaryDir mModuleDir;
    QTemporaryDir mOutputDir;

    static QByteArray moduleXml(const QString& aliasName)
    {
        return qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<!DOCTYPE MudletPackage>\n"
                   "<MudletPackage version=\"1.001\">\n"
                   "<AliasPackage>\n"
                   "<Alias isActive=\"yes\" isFolder=\"no\">\n"
                   "<name>%1</name>\n"
                   "<script></script>\n"
                   "<command></command>\n"
                   "<packageName></packageName>\n"
                   "<regex>^%1$</regex>\n"
                   "</Alias>\n"
                   "</AliasPackage>\n"
                   "</MudletPackage>\n")
                .arg(aliasName)
                .toUtf8();
    }

    static bool writeFile(const QString& path, const QByteArray& contents)
    {
        if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
            return false;
        }
        QFile file(path);
        if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
            return false;
        }
        return file.write(contents) == contents.size();
    }

    static QByteArray readFile(const QString& path)
    {
        QFile file(path);
        if (!file.open(QFile::ReadOnly)) {
            return {};
        }
        return file.readAll();
    }

    static bool writeArchive(const QString& path, const QString& entryName, const QByteArray& contents)
    {
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
        if (!archive) {
            return false;
        }
        zip_source* source = zip_source_buffer(archive, contents.constData(), contents.size(), 0);
        if (!source || zip_file_add(archive, entryName.toUtf8().constData(), source, ZIP_FL_ENC_UTF_8) < 0) {
            zip_source_free(source);
            zip_discard(archive);
            return false;
        }
        return zip_close(archive) == 0;
    }

    // Every entry of an archive, by name
    static QMap<QString, QByteArray> readArchive(const QString& path)
    {
        QMap<QString, QByteArray> entries;
        int errorCode = 0;
        zip* archive = zip_open(path.toUtf8().constData(), ZIP_RDONLY, &errorCode);
        if (!archive) {
            return entries;
        }
        for (zip_int64_t i = 0, total = zip_get_num_entries(archive, 0); i < total; ++i) {
            zip_stat_t entryStat;
            if (zip_stat_index(archive, static_cast<zip_uint64_t>(i), 0, &entryStat) != 0) {
                continue;
            }
            QByteArray contents;
            if (zip_file* file = zip_fopen_index(archive, static_cast<zip_uint64_t>(i), 0); file) {
                contents.resize(static_cast<qsizetype>(entryStat.size));
                zip_fread(file, contents.data(), entryStat.size);
                zip_fclose(file);
            }
            entries.insert(QString::fromUtf8(entryStat.name), contents);
        }
        zip_discard(archive);
        return entries;
    }

    // Runs the export the way the menu action does and hands back what it said
    std::tuple<bool, QString, QStringList> exportTo(const QString& archivePath)
    {
        MudletWebExport exporter(mpHost, archivePath);
        QSignalSpy finished(&exporter, &MudletWebExport::finished);
        exporter.start();
        if (finished.isEmpty() && !finished.wait(30s)) {
            return {false, qsl("the export never finished"), {}};
        }
        const auto arguments = finished.takeFirst();
        return {arguments.at(0).toBool(), arguments.at(1).toString(), arguments.at(2).toStringList()};
    }

    QString profileHome() const { return MudletApp::getMudletPath(enums::profileHomePath, mProfileName); }

    QString xmlModulePath() const { return mModuleDir.filePath(qsl("xml-module.xml")); }
    QString archivedModulePath() const { return mModuleDir.filePath(qsl("archived-module.mpackage")); }

    bool installModule(const QString& path)
    {
        mpHost->waitForProfileSave(); // an install during a save is postponed and answered with a bare true
        auto [installed, message] = mpHost->installPackage(path, enums::PackageModuleType::ModuleFromScript);
        if (!installed) {
            qWarning().noquote() << "installing" << path << "failed:" << message;
        }
        QTest::qWaitFor(
                [this]() {
                    return !mpHost->hasPendingProfileSave();
                },
                5s);
        mpHost->waitForProfileSave();
        return installed;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mModuleDir.isValid());
        QVERIFY(mOutputDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestProfile::removeProfileDirectory(mProfileName);

        mpHost = TestProfile::create(mProfileName, mLocalhost, QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "No active host after profile creation");

        QVERIFY(writeFile(xmlModulePath(), moduleXml(qsl("xml module alias"))));
        QVERIFY(writeArchive(archivedModulePath(), qsl("archived-module.xml"), moduleXml(qsl("archived module alias"))));
        QVERIFY2(installModule(xmlModulePath()), "The XML module could not be installed");
        QVERIFY2(installModule(archivedModulePath()), "The archived module could not be installed");

        QVERIFY(writeFile(qsl("%1/notes/todo.txt").arg(profileHome()), QByteArrayLiteral("kill the dragon")));
        QVERIFY(writeFile(qsl("%1/log/old-session.html").arg(profileHome()), QByteArrayLiteral("<html/>")));
        QVERIFY(writeFile(qsl("%1/media/cached.wav").arg(profileHome()), QByteArrayLiteral("RIFF")));
        QVERIFY(writeFile(qsl("%1/password").arg(profileHome()), QByteArrayLiteral("hunter2")));
        QVERIFY(writeFile(qsl("%1/current/2001-01-01#00-00-00.xml").arg(profileHome()), QByteArrayLiteral("<an old save/>")));
        QVERIFY(writeFile(qsl("%1/map/2001-01-01#00-00-00map.dat").arg(profileHome()), QByteArrayLiteral("an old map")));

        QVERIFY(mpHost->mpMap->addRoom(1));
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            TestProfile::removeProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_theArchiveHoldsTheProfileFolderWithItsModules()
    {
        const QString archivePath = mOutputDir.filePath(qsl("export.zip"));
        auto [ok, error, warnings] = exportTo(archivePath);
        QVERIFY2(ok, qPrintable(error));
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(qsl("; "))));

        const auto entries = readArchive(archivePath);
        const QString root = mProfileName + QLatin1Char('/');
        for (const auto& name : entries.keys()) {
            QVERIFY2(name.startsWith(root), qPrintable(qsl("\"%1\" is outside the profile's folder").arg(name)));
        }

        // Only the save just made: Mudlet Web takes the newest by file name, and
        // the older ones are nothing it can use
        QStringList saves;
        for (const auto& name : entries.keys()) {
            if (name.startsWith(root + qsl("current/"))) {
                saves << name;
            }
        }
        QCOMPARE(saves.size(), 1);
        QVERIFY2(!saves.first().endsWith(qsl("2001-01-01#00-00-00.xml")), "The old save went in instead of a fresh one");
        QVERIFY2(entries.value(saves.first()).contains("<key>xml-module</key>"), "The save does not list the XML module");

        // The map in memory, which has a room the old file on disk never had
        QStringList maps;
        for (const auto& name : entries.keys()) {
            if (name.startsWith(root + qsl("map/"))) {
                maps << name;
            }
        }
        QCOMPARE(maps.size(), 1);
        QVERIFY2(maps.first().endsWith(qsl("map.dat")), qPrintable(maps.first()));
        QVERIFY2(entries.value(maps.first()) != QByteArrayLiteral("an old map"), "The old map went in instead of the one in memory");

        // The module from outside the profile, under its own name
        QCOMPARE(entries.value(root + qsl("xml-module/xml-module.xml")), readFile(xmlModulePath()));
        // The archived one through desktop's unpacked copy, which already sits in
        // the profile, rather than a second copy of the archive
        QVERIFY2(entries.contains(root + qsl("archived-module/archived-module.xml")), "The archived module's unpacked XML is missing");
        QVERIFY2(!entries.contains(root + qsl("archived-module/archived-module.mpackage")), "The archived module went in twice");

        QCOMPARE(entries.value(root + qsl("notes/todo.txt")), QByteArrayLiteral("kill the dragon"));
        QVERIFY2(!entries.contains(root + qsl("password")), "A password file must never leave the machine in an export");
        for (const auto& name : entries.keys()) {
            QVERIFY2(!name.startsWith(root + qsl("log/")), qPrintable(qsl("Logs went in: %1").arg(name)));
            QVERIFY2(!name.startsWith(root + qsl("media/")), qPrintable(qsl("The media cache went in: %1").arg(name)));
        }
    }

    // Saved into the profile's own folder, a second export must not swallow the first
    void test_anExportInsideTheProfileDoesNotIncludeItself()
    {
        const QString archivePath = qsl("%1/exports/web.zip").arg(profileHome());
        QVERIFY(QDir().mkpath(QFileInfo(archivePath).absolutePath()));
        auto [firstOk, firstError, firstWarnings] = exportTo(archivePath);
        QVERIFY2(firstOk, qPrintable(firstError));
        auto [secondOk, secondError, secondWarnings] = exportTo(archivePath);
        QVERIFY2(secondOk, qPrintable(secondError));

        const auto entries = readArchive(archivePath);
        QVERIFY(!entries.isEmpty());
        QVERIFY2(!entries.contains(mProfileName + qsl("/exports/web.zip")), "The export packed a copy of itself");
        QFile::remove(archivePath);
    }

    void test_aModuleWhoseFileIsGoneIsReportedNotFatal()
    {
        const QString path = mModuleDir.filePath(qsl("vanishing-module.xml"));
        QVERIFY(writeFile(path, moduleXml(qsl("vanishing module alias"))));
        QVERIFY2(installModule(path), "The module could not be installed");
        QVERIFY(QFile::remove(path));

        const QString archivePath = mOutputDir.filePath(qsl("missing.zip"));
        auto [ok, error, warnings] = exportTo(archivePath);
        QVERIFY2(ok, qPrintable(error));
        QCOMPARE(warnings.size(), 1);
        QVERIFY2(warnings.first().contains(qsl("vanishing-module")), qPrintable(warnings.first()));
        QVERIFY(readArchive(archivePath).contains(mProfileName + qsl("/xml-module/xml-module.xml")));
    }

    void test_theSuggestedFileNameIsSafeOnEveryPlatform()
    {
        QCOMPARE(MudletWebExport::suggestedFileName(qsl("Achaea")), qsl("Achaea-mudlet-web.zip"));
        QCOMPARE(MudletWebExport::suggestedFileName(qsl("a/b\\c:d*e?f\"g<h>i|j")), qsl("a_b_c_d_e_f_g_h_i_j-mudlet-web.zip"));
    }
};

MUDLET_GROUPED_TEST_MAIN(MudletWebExportTest)
#include "MudletWebExportTest.moc"
