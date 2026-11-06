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

/*
 * Profile and module saves that the disk cannot hold whole. A full disk cuts a
 * write short without setting an error on the file, and a spec cannot fill a
 * disk, so each save runs under a file size limit on this process, which does
 * the same.
 *
 * Run with: ctest -R ProfileSaveDiskFullTest -V
 */

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QTimeZone>
#include <zip.h>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "mudlet.h"

#include "GroupedTest.h"

#if defined(Q_OS_UNIX)
#include <algorithm>
#include <csignal>
#include <sys/resource.h>
#endif

using namespace std::chrono_literals;

namespace {
constexpr qint64 scmFileSizeLimit = 20 * 1024;

// With the default count, well past the limit once saved
QByteArray packageWithTriggers(const QString& namePrefix, const int count = 100)
{
    QByteArray xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE MudletPackage>\n<MudletPackage version=\"1.001\">\n<TriggerPackage>\n";
    for (int i = 1; i <= count; ++i) {
        xml += qsl("<Trigger isActive=\"yes\" isFolder=\"no\" isTempTrigger=\"no\" isMultiline=\"no\" isPerlSlashGOption=\"no\" isColorizerTrigger=\"no\" isFilterTrigger=\"no\" "
                   "isSoundTrigger=\"no\" isColorTrigger=\"no\" isColorTriggerFg=\"no\" isColorTriggerBg=\"no\"><name>%1 %2</name><script>echo(\"%1 %2\")</script>"
                   "<triggerType>0</triggerType><conditonLineDelta>0</conditonLineDelta><mStayOpen>0</mStayOpen><mCommand></mCommand><packageName></packageName>"
                   "<mFgColor>#ff0000</mFgColor><mBgColor>#ffff00</mBgColor><mSoundFile></mSoundFile><colorTriggerFgColor>#000000</colorTriggerFgColor>"
                   "<colorTriggerBgColor>#000000</colorTriggerBgColor><regexCodeList><string>^%1 pattern %2$</string></regexCodeList>"
                   "<regexCodePropertyList><integer>1</integer></regexCodePropertyList></Trigger>\n")
                       .arg(namePrefix)
                       .arg(i)
                       .toUtf8();
    }
    xml += "</TriggerPackage>\n</MudletPackage>\n";
    return xml;
}

bool writeArchive(const QString& path, const QMap<QString, QByteArray>& entries)
{
    int errorCode = 0;
    zip* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errorCode);
    if (!archive) {
        return false;
    }
    for (auto [entryName, contents] : entries.asKeyValueRange()) {
        zip_source* source = zip_source_buffer(archive, contents.constData(), contents.size(), 0);
        if (!source || zip_file_add(archive, entryName.toUtf8().constData(), source, ZIP_FL_ENC_UTF_8) < 0) {
            zip_source_free(source);
            zip_discard(archive);
            return false;
        }
    }
    return zip_close(archive) == 0;
}

QByteArray archiveEntry(const QString& path, const QString& entryName)
{
    int errorCode = 0;
    zip* archive = zip_open(path.toUtf8().constData(), ZIP_RDONLY, &errorCode);
    if (!archive) {
        return {};
    }
    QByteArray contents;
    zip_stat_t entryStat;
    if (zip_stat(archive, entryName.toUtf8().constData(), 0, &entryStat) == 0) {
        if (zip_file* file = zip_fopen(archive, entryName.toUtf8().constData(), 0); file) {
            contents.resize(static_cast<qsizetype>(entryStat.size));
            if (zip_fread(file, contents.data(), entryStat.size) != static_cast<zip_int64_t>(entryStat.size)) {
                contents.clear();
            }
            zip_fclose(file);
        }
    }
    zip_discard(archive);
    return contents;
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
} // namespace

class ProfileSaveDiskFullTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("ProfileSaveDiskFull-Test");
    QTemporaryDir mConfigDir;
    QTemporaryDir mModuleDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    int mConsoleMark = 0;

    QString savesFolder() const { return MudletApp::getMudletPath(enums::profileXmlFilesPath, mProfileName); }
    bool lua(const QString& code) const { return mpHost->getLuaInterpreter()->compileAndExecuteScript(code); }

    void markConsole() { mConsoleMark = mpHost->mainConsoleView()->buffer.getLastLineNumber(); }

    QString consoleSinceMark() const
    {
        QStringList lines;
        auto& buffer = mpHost->mainConsoleView()->buffer;
        for (int line = std::max(0, mConsoleMark); line <= buffer.getLastLineNumber(); ++line) {
            lines << buffer.line(line);
        }
        return lines.join(QChar::Space).simplified();
    }

    bool saveAndWait(const QString& saveName)
    {
        auto [ok, filename, error] = mpHost->saveProfile(QString(), saveName);
        if (!ok) {
            qWarning() << "ProfileSaveDiskFullTest - the save was refused:" << error;
            return false;
        }
        return QTest::qWaitFor(
                [this]() {
                    return !mpHost->currentlySavingProfile();
                },
                10s);
    }

#if defined(Q_OS_UNIX)
    // The save is waited on with the limit in place, so its writes on the pool thread land while it holds
    bool saveUnderFileSizeLimit(const QString& saveName, const qint64 limit = scmFileSizeLimit)
    {
        rlimit saved{};
        if (::getrlimit(RLIMIT_FSIZE, &saved) != 0) {
            return false;
        }
        rlimit limited = saved;
        limited.rlim_cur = std::min<rlim_t>(limit, saved.rlim_max);
        // Past the limit the kernel also raises SIGXFSZ, whose default action ends the process
        const auto previousHandler = std::signal(SIGXFSZ, SIG_IGN);
        bool finished = false;
        if (::setrlimit(RLIMIT_FSIZE, &limited) == 0) {
            finished = saveAndWait(saveName);
            finished = ::setrlimit(RLIMIT_FSIZE, &saved) == 0 && finished;
        }
        std::signal(SIGXFSZ, previousHandler);
        return finished;
    }
#endif

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mModuleDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(MudletApp::getMudletPath(enums::profilesPath).startsWith(mConfigDir.path()), "the config dir was not redirected");

        QVERIFY(QDir().mkpath(savesFolder()));
        QFile save(QDir(savesFolder()).filePath(qsl("2026-01-01#00-00-00.xml")));
        QVERIFY(save.open(QIODevice::WriteOnly));
        QVERIFY(save.write(packageWithTriggers(qsl("profile trigger"))) > 0);
        save.close();
        mudlet::self()->doAutoLogin(mProfileName, true);
        mpHost = HostManager::self()->getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(mpHost->mLoadedOk);
        QVERIFY(mpHost->mainConsoleView());
        // Wrapped, a message would be split mid-path
        QVERIFY(lua(qsl("setWindowWrap('main', 2000)")));
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_aProfileSaveTheDiskCannotHoldLeavesNoCutOffSave()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to cut a write short");
#else
        QVERIFY(saveAndWait(qsl("kept")));
        QFile kept(QDir(savesFolder()).filePath(qsl("kept.xml")));
        QVERIFY(kept.open(QIODevice::ReadOnly));
        QVERIFY2(kept.size() > scmFileSizeLimit, "the profile saves whole within the limit, so the limit cuts nothing");
        markConsole();

        QVERIFY2(saveUnderFileSizeLimit(qsl("cut-short")), "the save never finished");

        // Not even a temporary of it is left, which the loader would skip anyway
        const QStringList left = QDir(savesFolder()).entryList({qsl("cut-short*")}, QDir::Files | QDir::Hidden);
        QVERIFY2(left.isEmpty(), qPrintable(qsl("the part of the save that fit was kept: %1").arg(left.join(qsl(", ")))));
        const QString reason = qsl("could not be saved to \"%1\": only %2 of").arg(QDir(savesFolder()).filePath(qsl("cut-short.xml"))).arg(scmFileSizeLimit);
        QVERIFY2(consoleSinceMark().contains(reason), qPrintable(consoleSinceMark()));
#endif
    }

    // A synced module is saved over the file it is loaded from, where a profile save gets a new file each time
    void test_aSyncedModuleTheDiskCannotHoldKeepsItsFile()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to cut a write short");
#else
        const QString moduleFile = mModuleDir.filePath(qsl("diskfullmodule.xml"));
        QFile module(moduleFile);
        QVERIFY(module.open(QIODevice::WriteOnly));
        QVERIFY(module.write(packageWithTriggers(qsl("module trigger"))) > 0);
        module.close();
        QVERIFY(lua(qsl("installModule([[%1]])").arg(moduleFile)));
        QVERIFY(lua(qsl("enableModuleSync('diskfullmodule')")));
        // A save first, so the file holds what Mudlet writes and what is kept can be compared to it
        QVERIFY(saveAndWait(qsl("module-written")));
        const QByteArray written = readFile(moduleFile);
        QVERIFY2(written.size() > scmFileSizeLimit, "the module saves whole within the limit, so the limit cuts nothing");
        markConsole();

        QVERIFY2(saveUnderFileSizeLimit(qsl("module-cut-short")), "the save never finished");

        QVERIFY2(readFile(moduleFile) == written, qPrintable(qsl("the module's file was replaced, it is now %1 bytes").arg(QFileInfo(moduleFile).size())));
        const QString reason = qsl("The module \"diskfullmodule\" could not be saved to \"%1\": only %2 of").arg(moduleFile).arg(scmFileSizeLimit);
        QVERIFY2(consoleSinceMark().contains(reason), qPrintable(consoleSinceMark()));
#endif
    }

    // Small enough for QFileDevice's 16 KiB write buffer, which would hold the write back until commit()
    void test_aSmallModuleTheDiskCannotHoldSaysWhy()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to cut a write short");
#else
        constexpr qint64 limit = 2 * 1024;
        const QString moduleFile = mModuleDir.filePath(qsl("diskfullsmallmodule.xml"));
        QFile module(moduleFile);
        QVERIFY(module.open(QIODevice::WriteOnly));
        QVERIFY(module.write(packageWithTriggers(qsl("small module trigger"), 10)) > 0);
        module.close();
        QVERIFY(lua(qsl("installModule([[%1]])").arg(moduleFile)));
        QVERIFY(lua(qsl("enableModuleSync('diskfullsmallmodule')")));
        QVERIFY(saveAndWait(qsl("small-module-written")));
        const QByteArray written = readFile(moduleFile);
        QVERIFY2(written.size() > limit && written.size() < 16 * 1024, qPrintable(qsl("the module saves to %1 bytes").arg(written.size())));
        markConsole();

        QVERIFY2(saveUnderFileSizeLimit(qsl("small-module-cut-short"), limit), "the save never finished");

        QVERIFY2(readFile(moduleFile) == written, qPrintable(qsl("the module's file was replaced, it is now %1 bytes").arg(QFileInfo(moduleFile).size())));
        const QString reason = qsl("The module \"diskfullsmallmodule\" could not be saved to \"%1\": only %2 of").arg(moduleFile).arg(limit);
        QVERIFY2(consoleSinceMark().contains(reason), qPrintable(consoleSinceMark()));
#endif
    }

    // The unpacked XML is too big for the limit, the archive it is packed into is not
    void test_aSyncedArchiveModuleTheDiskCannotHoldKeepsItsArchive()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to cut a write short");
#else
        const QString archiveFile = mModuleDir.filePath(qsl("diskfullarchive.mpackage"));
        const QString entryName = qsl("diskfullarchive.xml");
        QVERIFY(writeArchive(archiveFile, {{entryName, packageWithTriggers(qsl("archive trigger"))}}));
        QVERIFY(lua(qsl("installModule([[%1]])").arg(archiveFile)));
        QVERIFY(lua(qsl("enableModuleSync('diskfullarchive')")));
        QVERIFY(saveAndWait(qsl("archive-written")));
        const QByteArray written = archiveEntry(archiveFile, entryName);
        QVERIFY2(written.size() > scmFileSizeLimit, "the module saves whole within the limit, so the limit cuts nothing");
        QVERIFY2(QFileInfo(archiveFile).size() < scmFileSizeLimit, "the archive does not fit within the limit, so it cannot be rebuilt from a cut-off XML");
        const QDateTime archiveTime(QDate(2020, 1, 1), QTime(0, 0), QTimeZone::UTC);
        QFile archive(archiveFile);
        QVERIFY(archive.open(QIODevice::ReadWrite) && archive.setFileTime(archiveTime, QFileDevice::FileModificationTime));
        archive.close();
        markConsole();

        QVERIFY2(saveUnderFileSizeLimit(qsl("archive-cut-short")), "the save never finished");

        const QByteArray kept = archiveEntry(archiveFile, entryName);
        QVERIFY2(kept == written, qPrintable(qsl("the module's archive now holds %1 bytes of its XML").arg(kept.size())));
        const QString unpacked = MudletApp::getMudletPath(enums::profilePackagePathFileName, mProfileName, qsl("diskfullarchive"));
        QVERIFY2(readFile(unpacked) == written, qPrintable(qsl("the module's unpacked XML was replaced, it is now %1 bytes").arg(QFileInfo(unpacked).size())));
        QVERIFY2(consoleSinceMark().contains(qsl("The module \"diskfullarchive\" could not be saved to")), qPrintable(consoleSinceMark()));
        // Not rebuilt either, as the XML this save meant to put in it never landed
        QVERIFY2(QFileInfo(archiveFile).lastModified(QTimeZone::UTC) == archiveTime, "the module's archive was rebuilt from an XML that was not saved");
#endif
    }

    // A full disk is more likely to refuse the archive, a copy of the module's every file, than its XML
    void test_anArchiveModuleWhoseArchiveTheDiskCannotHoldSaysSo()
    {
#if !defined(Q_OS_UNIX)
        QSKIP("needs RLIMIT_FSIZE to cut a write short");
#else
        constexpr qint64 limit = 160 * 1024;
        const QString archiveFile = mModuleDir.filePath(qsl("diskfullbigarchive.mpackage"));
        const QString entryName = qsl("diskfullbigarchive.xml");
        QByteArray noise(256 * 1024, Qt::Uninitialized);
        QRandomGenerator(12345).fillRange(reinterpret_cast<quint32*>(noise.data()), noise.size() / sizeof(quint32));
        QVERIFY(writeArchive(archiveFile, {{entryName, packageWithTriggers(qsl("big archive trigger"))}, {qsl("noise.bin"), noise}}));
        QVERIFY(lua(qsl("installModule([[%1]])").arg(archiveFile)));
        QVERIFY(lua(qsl("enableModuleSync('diskfullbigarchive')")));
        QVERIFY(saveAndWait(qsl("big-archive-written")));
        const QByteArray written = archiveEntry(archiveFile, entryName);
        QVERIFY2(!written.isEmpty() && written.size() < limit, "the module's XML does not fit within the limit");
        QVERIFY2(QFileInfo(archiveFile).size() > limit, "the archive fits within the limit, so the limit cuts nothing");
        markConsole();

        QVERIFY2(saveUnderFileSizeLimit(qsl("big-archive-cut-short"), limit), "the save never finished");

        QVERIFY2(archiveEntry(archiveFile, entryName) == written, "the module's archive lost its XML");
        const QString reason = qsl("The module \"diskfullbigarchive\" could not be saved to \"%1\":").arg(archiveFile);
        QVERIFY2(consoleSinceMark().contains(reason), qPrintable(consoleSinceMark()));
#endif
    }
};

#include "ProfileSaveDiskFullTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileSaveDiskFullTest)
