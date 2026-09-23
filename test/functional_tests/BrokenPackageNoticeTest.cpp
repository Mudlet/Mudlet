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
 * An item whose Lua does not work is kept, so that it can be fixed in the
 * editor, and the install carries on around it - which used to make a package
 * that is not running look exactly like a healthy one. Host::installPackage()
 * now says so, and who hears it is the point of this test:
 *
 *  - the install a person asked for and is watching says it on the console, by
 *    name, and leaves the Lua error text to the editor, which shows it against
 *    the item itself;
 *  - an install a script asked for says nothing there - the reason is handed
 *    back to the caller and to the install event, which is where a package
 *    manager reads it;
 *  - a profile opening says nothing either, because every module it has is
 *    reinstalled from its archive as it opens (mudlet::installModulesList()) -
 *    saying it there brings the line back on every launch for as long as the
 *    module is broken, at somebody who, for a module they did not write, can
 *    do nothing about it.
 *
 * This cannot be a spec: installPackage() and installModule() both install
 * quietly (TLuaInterpreter passes quiet = true), so the console line has no Lua
 * entry point at all, and neither has the profile-loading flag.
 *
 * Run with: ctest -R BrokenPackageNoticeTest -V
 */

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletPaths.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include "GroupedTest.h"

class BrokenPackageNoticeTest : public QObject
{
    Q_OBJECT

private:
    const QString mHostname = qsl("BrokenPackageInstallNotice-Test");
    const QString mLocalhost = qsl("127.0.0.1");
    QString mPort;
    QTemporaryDir mConfigDir;
    QTemporaryDir mPackageDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;

    // One trigger whose body will not compile, one script that stops with an
    // error as it is read in, and one script with nothing wrong with it - so a
    // partial failure can be told from a whole one.
    static QByteArray packageXml(const QString& name)
    {
        return qsl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<!DOCTYPE MudletPackage>\n"
                   "<MudletPackage version=\"1.001\">\n"
                   "<TriggerPackage>\n"
                   "<Trigger isActive=\"yes\" isFolder=\"no\" isTempTrigger=\"no\" isMultiline=\"no\" isPerlSlashGOption=\"no\"\n"
                   "         isColorizerTrigger=\"no\" isFilterTrigger=\"no\" isSoundTrigger=\"no\" isColorTrigger=\"no\">\n"
                   "<name>%1 trigger</name><script>this is not lua(</script>\n"
                   "<triggerType>0</triggerType><conditonLineDelta>0</conditonLineDelta><mStayOpen>0</mStayOpen>\n"
                   "<mCommand></mCommand><packageName></packageName>\n"
                   "<regexCodeList><string>%1 pattern</string></regexCodeList>\n"
                   "<regexCodePropertyList><integer>0</integer></regexCodePropertyList>\n"
                   "</Trigger>\n"
                   "</TriggerPackage>\n"
                   "<ScriptPackage>\n"
                   "<Script isActive=\"yes\" isFolder=\"no\">\n"
                   "<name>%1 script</name><packageName></packageName>\n"
                   "<script>brokenPackageNoticeMissingFunction()</script>\n"
                   "<eventHandlerList />\n"
                   "</Script>\n"
                   "<Script isActive=\"yes\" isFolder=\"no\">\n"
                   "<name>%1 sibling</name><packageName></packageName>\n"
                   "<script>brokenPackageNoticeSiblingRan = true</script>\n"
                   "<eventHandlerList />\n"
                   "</Script>\n"
                   "</ScriptPackage>\n"
                   "</MudletPackage>\n")
                .arg(name)
                .toUtf8();
    }

    QString writePackage(const QString& name)
    {
        const QString path = mPackageDir.filePath(qsl("%1.xml").arg(name));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return QString();
        }
        file.write(packageXml(name));
        file.close();
        return path;
    }

    static int consoleMark(Host* host) { return host->mpConsole->buffer.getLastLineNumber(); }

    static QString consoleTextSince(Host* host, const int mark)
    {
        QString text;
        for (int i = mark; i <= host->mpConsole->buffer.getLastLineNumber(); ++i) {
            text.append(host->mpConsole->buffer.line(i)).append(QChar::Space);
        }
        return text.simplified();
    }

    static void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletPaths::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    Host* startProfile()
    {
        Host* host = TestProfile::create(mHostname, mLocalhost, mPort);
        if (host) {
            // The profile has finished loading by now, but say so out loud: every
            // assertion below is about what an install does outside that window.
            host->mIsProfileLoadingSequence = false;
        }
        return host;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(mPackageDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // The install someone chose from the package manager, or dropped onto the
    // window: it is theirs, they are looking at it, and it is said once.
    void test_anInstallAPersonAskedForNamesWhatIsNotWorking()
    {
        Host* host = startProfile();
        QVERIFY2(host, "no active host available for the test");
        const QString name = qsl("broken-notice-package");
        const QString path = writePackage(name);
        QVERIFY(!path.isEmpty());

        const int mark = consoleMark(host);
        auto [ok, reason] = host->installPackage(path, enums::PackageModuleType::Package, false);
        const QString said = consoleTextSince(host, mark);

        QVERIFY2(ok, qPrintable(qsl("the package did not install: %1").arg(reason)));
        QVERIFY2(said.contains(qsl("was installed, but these parts of it are not working")), qPrintable(qsl("the console said: \"%1\"").arg(said)));
        QVERIFY2(said.contains(qsl("%1 script").arg(name)), qPrintable(qsl("the script that stopped was not named: \"%1\"").arg(said)));
        QVERIFY2(said.contains(qsl("%1 trigger").arg(name)), qPrintable(qsl("the trigger that will not compile was not named: \"%1\"").arg(said)));
    }

    // The Lua error names a line in an item the player did not write. It is of
    // use to whoever did, so it is left to the editor - and to the return value
    // and the install event, which this checks is still carrying it.
    void test_theConsoleLineLeavesTheLuaErrorToTheEditor()
    {
        Host* host = startProfile();
        QVERIFY2(host, "no active host available for the test");
        const QString name = qsl("broken-notice-wording");
        const QString path = writePackage(name);
        QVERIFY(!path.isEmpty());

        const int mark = consoleMark(host);
        auto [ok, reason] = host->installPackage(path, enums::PackageModuleType::Package, false);
        const QString said = consoleTextSince(host, mark);

        QVERIFY(ok);
        QVERIFY2(!said.contains(qsl("Lua syntax error")), qPrintable(qsl("the console repeated the Lua error text: \"%1\"").arg(said)));
        QVERIFY2(!said.contains(qsl("[string \"")), qPrintable(qsl("the console repeated the Lua error text: \"%1\"").arg(said)));
        QVERIFY2(reason.contains(qsl("brokenPackageNoticeMissingFunction")), qPrintable(qsl("the caller was not given the reason: \"%1\"").arg(reason)));
        QVERIFY2(reason.contains(qsl("%1 trigger").arg(name)), qPrintable(qsl("the caller was not told about the trigger: \"%1\"").arg(reason)));
    }

    // An install a script asked for. mpkg updates packages this way, on every
    // startup, and has the return value to report from - so the console stays
    // out of it.
    void test_aScriptedInstallIsLeftToItsReturnValue()
    {
        Host* host = startProfile();
        QVERIFY2(host, "no active host available for the test");
        const QString name = qsl("broken-notice-quiet");
        const QString path = writePackage(name);
        QVERIFY(!path.isEmpty());

        const int mark = consoleMark(host);
        auto [ok, reason] = host->installPackage(path, enums::PackageModuleType::Package, true);
        const QString said = consoleTextSince(host, mark);

        QVERIFY2(ok, qPrintable(qsl("the package did not install: %1").arg(reason)));
        QVERIFY2(!said.contains(qsl("are not working")), qPrintable(qsl("a quiet install still said it on the console: \"%1\"").arg(said)));
        QVERIFY2(reason.contains(qsl("%1 script").arg(name)), qPrintable(qsl("the caller was not given the reason: \"%1\"").arg(reason)));
    }

    // Opening a profile reinstalls every module it has, so this is the install
    // that would repeat the line on every launch. The reason still has to reach
    // the caller, which is what tells this apart from the line never being
    // worked out at all.
    void test_aProfileOpeningItsModulesSaysNothing()
    {
        Host* host = startProfile();
        QVERIFY2(host, "no active host available for the test");
        const QString name = qsl("broken-notice-module");
        const QString path = writePackage(name);
        QVERIFY(!path.isEmpty());

        // what mudlet::openProfile() has set while installModulesList() runs
        host->mIsProfileLoadingSequence = true;
        const int mark = consoleMark(host);
        auto [ok, reason] = host->installPackage(path, enums::PackageModuleType::ModuleFromUI, false);
        const QString said = consoleTextSince(host, mark);
        host->mIsProfileLoadingSequence = false;

        QVERIFY2(ok, qPrintable(qsl("the module did not install: %1").arg(reason)));
        QVERIFY2(!said.contains(qsl("are not working")), qPrintable(qsl("a profile opening its modules said it again: \"%1\"").arg(said)));
        QVERIFY2(reason.contains(qsl("%1 script").arg(name)), qPrintable(qsl("the caller was not given the reason: \"%1\"").arg(reason)));
    }

    // The same module installed from the module manager, which is a person
    // asking for it: that one is said.
    void test_aModuleAPersonInstallsIsStillNamed()
    {
        Host* host = startProfile();
        QVERIFY2(host, "no active host available for the test");
        const QString name = qsl("broken-notice-module-by-hand");
        const QString path = writePackage(name);
        QVERIFY(!path.isEmpty());

        const int mark = consoleMark(host);
        auto [ok, reason] = host->installPackage(path, enums::PackageModuleType::ModuleFromUI, false);
        const QString said = consoleTextSince(host, mark);

        QVERIFY2(ok, qPrintable(qsl("the module did not install: %1").arg(reason)));
        QVERIFY2(said.contains(qsl("Module \"%1\" was installed, but these parts of it are not working").arg(name)), qPrintable(qsl("the console said: \"%1\"").arg(said)));
    }
};

#include "BrokenPackageNoticeTest.moc"
MUDLET_GROUPED_TEST_MAIN(BrokenPackageNoticeTest)
