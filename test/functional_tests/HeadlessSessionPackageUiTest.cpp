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
 * Which package made each window a profile's scripts made, and whether that package says its interface suits a
 * phone, as THeadlessSession gives them to an app on a small screen. Each test makes a session of its own, and with
 * it the process's one profile pool. A spec cannot reach these: the makers are kept in C++ and read only through
 * THeadlessSession.
 *
 * Run with: ctest -R HeadlessSessionPackageUiTest -V
 */

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <optional>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "THeadlessSession.h"

#include "GroupedTest.h"

class HeadlessSessionPackageUiTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    static bool writeFile(const QString& path, const QByteArray& contents)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }

    static bool writeProfile(const QString& name)
    {
        const QString saveFolder = MudletApp::getMudletPath(enums::profileXmlFilesPath, name);
        return QDir().mkpath(saveFolder) && writeFile(qsl("%1/2020-01-01#00-00-00.xml").arg(saveFolder), R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<HostPackage><Host></Host></HostPackage>
</MudletPackage>
)");
    }

    // One script, named as given: two packages with a script of one name are the collision the makers allow for
    static QByteArray packageXml(const QByteArray& scriptName, const QByteArray& script)
    {
        return R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
<ScriptPackage><Script isActive="yes" isFolder="no"><name>)"
               + scriptName + R"(</name><packageName></packageName><script>)" + script + R"(</script><eventHandlerList/></Script></ScriptPackage>
</MudletPackage>
)";
    }

    // The package is named after the file
    bool install(THeadlessSession& session, const QString& package, const QByteArray& scriptName, const QByteArray& script)
    {
        const QString path = mConfigDir.filePath(qsl("%1.xml").arg(package));
        if (!writeFile(path, packageXml(scriptName, script))) {
            return false;
        }
        const THeadlessSession::PackageResult result = session.installPackage(path);
        if (result != THeadlessSession::PackageResult::installed) {
            qWarning().noquote() << "installing" << package << "failed:" << session.errorString();
            return false;
        }
        return true;
    }

    static QStringList makersOf(const THeadlessSession& session, const QString& name, std::optional<THeadlessSession::ScriptWindow::Kind> kind = std::nullopt)
    {
        const std::optional<THeadlessSession::ScriptWindow> window = session.scriptWindow(name, kind);
        return window ? window->packages : QStringList{qsl("<no window named %1>").arg(name)};
    }

    static bool mobileOf(const THeadlessSession& session, const QString& name)
    {
        const std::optional<THeadlessSession::ScriptWindow> window = session.scriptWindow(name);
        return window && window->mobile;
    }

    static QStringList namesTold(const QSignalSpy& spy)
    {
        QStringList names;
        for (const QList<QVariant>& emitted : spy) {
            names << emitted.at(1).toStringList();
        }
        return names;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        QVERIFY2(!HostManager::self(), "A profile pool already exists, so a session cannot make its own.");
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    // Geyser's own code is no package's, so what it makes is the package's that called it
    void test_whatAPackagesScriptMakesIsThePackages()
    {
        const QString name = qsl("Headless Package Ui Made");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(install(session, qsl("madeByPackage"), "madeByPackageScript", R"(
createLabel("pkgLabel", 0, 0, 10, 10, 1)
createMiniConsole("pkgConsole", 0, 20, 100, 50)
enableCommandLine("pkgConsole")
openUserWindow("pkgWindow", false)
createScrollBox("pkgBox", 0, 80, 100, 50)
createCommandLine("pkgLine", 0, 140, 100, 20)
createTextEdit("pkgText", 0, 170, 100, 20)
pkgGeyser = Geyser.Label:new({name = "pkgGeyser", x = 0, y = 200, width = 50, height = 10})
pkgGauge = Geyser.Gauge:new({name = "pkgGauge", x = 0, y = 220, width = 50, height = 10})
)"));
        QVERIFY(session.runLua(qsl(R"(createLabel("ownLabel", 0, 300, 10, 10, 1))")));

        using Kind = THeadlessSession::ScriptWindow::Kind;
        const QStringList package{qsl("madeByPackage")};
        for (const QString& window : {qsl("pkgLabel"),
                                      qsl("pkgConsole"),
                                      qsl("pkgWindow"),
                                      qsl("pkgBox"),
                                      qsl("pkgLine"),
                                      qsl("pkgText"),
                                      qsl("pkgGeyser"),
                                      qsl("pkgGauge_back"),
                                      qsl("pkgGauge_front"),
                                      qsl("pkgGauge_text")}) {
            QVERIFY2(makersOf(session, window) == package, qPrintable(qsl("%1 is made by %2").arg(window, makersOf(session, window).join(qsl(", ")))));
            QVERIFY2(!mobileOf(session, window), qPrintable(qsl("%1, from a package that says nothing, is said to suit a phone").arg(window)));
        }
        QCOMPARE(makersOf(session, qsl("pkgConsole"), Kind::commandLine), package);
        QCOMPARE(makersOf(session, qsl("ownLabel")), QStringList());
        QVERIFY(mobileOf(session, qsl("ownLabel")));
        QVERIFY(session.shutdown(false));
    }

    // A handler a package registered runs its code, whoever raised the event; a string compiled with loadstring() is
    // what the player typed, as a "lua" alias runs it
    void test_aHandlerIsItsPackagesAndCompiledStringsAreTheProfiles()
    {
        const QString name = qsl("Headless Package Ui Handler");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(install(session, qsl("handlerPackage"), "handlerPackageScript", R"(
registerAnonymousEventHandler("handlerPackageMake", function() createLabel("handlerLabel", 0, 0, 10, 10, 1) end)
function handlerPackageRun(code) assert(loadstring(code))() end
)"));
        QVERIFY(session.runLua(qsl(R"(raiseEvent("handlerPackageMake"))")));
        QCOMPARE(makersOf(session, qsl("handlerLabel")), QStringList{qsl("handlerPackage")});
        QVERIFY(session.runLua(qsl(R"(handlerPackageRun([[createLabel("typedLabel", 0, 20, 10, 10, 1)]]))")));
        QCOMPARE(makersOf(session, qsl("typedLabel")), QStringList());
        QVERIFY(session.shutdown(false));
    }

    void test_aFunctionTimerIsThePackagesAndAStringTimerIsNot()
    {
        const QString name = qsl("Headless Package Ui Timer");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(install(session, qsl("timerPackage"), "timerPackageScript", R"(
tempTimer(0, function() createLabel("functionTimerLabel", 0, 0, 10, 10, 1) end)
tempTimer(0, [[createLabel("stringTimerLabel", 0, 20, 10, 10, 1)]])
)"));
        QTRY_VERIFY(session.scriptWindow(qsl("functionTimerLabel")) && session.scriptWindow(qsl("stringTimerLabel")));
        QCOMPARE(makersOf(session, qsl("functionTimerLabel")), QStringList{qsl("timerPackage")});
        QCOMPARE(makersOf(session, qsl("stringTimerLabel")), QStringList());
        QVERIFY(session.shutdown(false));
    }

    void test_aLibraryCountsAsThePackageThatListsItAsADependency()
    {
        const QString name = qsl("Headless Package Ui Library");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(install(session, qsl("libraryPackage"), "libraryPackageScript", R"(
function libraryMake(label) createLabel(label, 0, 0, 10, 10, 1) end
)"));
        QVERIFY(install(session, qsl("needsLibrary"), "needsLibraryScript", R"(
registerAnonymousEventHandler("needsLibraryMake", function() libraryMake("forTheNeedingOne") end)
)"));
        QVERIFY(install(session, qsl("ignoresLibrary"), "ignoresLibraryScript", R"(
registerAnonymousEventHandler("ignoresLibraryMake", function() libraryMake("forTheOtherOne") end)
)"));
        QVERIFY(session.runLua(qsl(R"(setPackageInfo("needsLibrary", "dependencies", "someOther, libraryPackage"))")));
        QVERIFY(session.runLua(qsl(R"(raiseEvent("needsLibraryMake") raiseEvent("ignoresLibraryMake"))")));
        QCOMPARE(makersOf(session, qsl("forTheNeedingOne")), QStringList{qsl("needsLibrary")});
        QCOMPARE(makersOf(session, qsl("forTheOtherOne")), QStringList{qsl("libraryPackage")});
        QVERIFY(session.shutdown(false));
    }

    // Two packages each with a script of one name: the window is either's, and so suits a phone if either says so
    void test_aCollisionSuitsAPhoneOnlyOnceOneOfThemSaysSo()
    {
        const QString name = qsl("Headless Package Ui Collision");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(install(session, qsl("collidingFirst"), "sharedScriptName", R"(
registerAnonymousEventHandler("collidingMake", function() createLabel("collidedLabel", 0, 0, 10, 10, 1) end)
)"));
        QVERIFY(install(session, qsl("collidingSecond"), "sharedScriptName", R"(collidingSecondLoaded = true)"));
        QVERIFY(session.runLua(qsl(R"(raiseEvent("collidingMake"))")));
        QCOMPARE(makersOf(session, qsl("collidedLabel")), (QStringList{qsl("collidingFirst"), qsl("collidingSecond")}));
        QVERIFY(!mobileOf(session, qsl("collidedLabel")));
        QVERIFY(!session.packageMobile(qsl("collidingSecond")));

        QSignalSpy changes(&session, &THeadlessSession::scriptWindowsChanged);
        QSignalSpy packageUi(&session, &THeadlessSession::packageUiChanged);
        QVERIFY(session.runLua(qsl(R"(setPackageInfo("collidingSecond", "mobile", " Yes "))")));
        QTRY_COMPARE(packageUi.count(), 1);
        QVERIFY(session.packageMobile(qsl("collidingSecond")));
        QTRY_VERIFY(namesTold(changes).contains(qsl("collidedLabel")));
        QVERIFY(mobileOf(session, qsl("collidedLabel")));
        QVERIFY(session.shutdown(false));
    }

    // Set as the window is made: making it again moves it, and only a delete forgets who made it
    void test_aWindowKeepsItsMakersUntilItIsDeleted()
    {
        const QString name = qsl("Headless Package Ui Kept");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        QVERIFY(install(session, qsl("keptPackage"), "keptPackageScript", R"(
createLabel("keptLabel", 0, 0, 10, 10, 1)
createMiniConsole("keptConsole", 0, 20, 100, 50)
)"));
        QVERIFY(session.runLua(qsl(R"(
createLabel("keptLabel", 5, 5, 20, 20, 1)
moveWindow("keptLabel", 30, 30)
createMiniConsole("keptConsole", 0, 40, 100, 50)
)")));
        QCOMPARE(makersOf(session, qsl("keptLabel")), QStringList{qsl("keptPackage")});
        QCOMPARE(makersOf(session, qsl("keptConsole")), QStringList{qsl("keptPackage")});
        QVERIFY(session.runLua(qsl(R"(
deleteLabel("keptLabel")
createLabel("keptLabel", 0, 0, 10, 10, 1)
)")));
        QCOMPARE(makersOf(session, qsl("keptLabel")), QStringList());
        QVERIFY(session.shutdown(false));
    }

    void test_aModulesWindowsAreItsOwn()
    {
        const QString name = qsl("Headless Package Ui Module");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        const QString path = mConfigDir.filePath(qsl("uiModule.xml"));
        QVERIFY(writeFile(path, packageXml("uiModuleScript", R"(createLabel("moduleLabel", 0, 0, 10, 10, 1))")));
        const auto [installed, why] = session.host()->installPackage(path, enums::PackageModuleType::ModuleFromUI);
        QVERIFY2(installed, qPrintable(why));
        QCOMPARE(makersOf(session, qsl("moduleLabel")), QStringList{qsl("uiModule")});
        QVERIFY(!mobileOf(session, qsl("moduleLabel")));
        QVERIFY(session.runLua(qsl(R"(setModuleInfo("uiModule", "mobile", "true"))")));
        QVERIFY(session.packageMobile(qsl("uiModule")));
        QVERIFY(mobileOf(session, qsl("moduleLabel")));
        QVERIFY(session.shutdown(false));
    }

    // A boolean was dropped before, so a package that writes none gains no detail
    void test_configLuaDeclaresWithYesOrTrue()
    {
        const QString name = qsl("Headless Package Ui Config");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        Host* host = session.host();
        const auto readConfig = [this, host](const QString& package, const QByteArray& body) {
            const QString path = mConfigDir.filePath(qsl("%1-config.lua").arg(package));
            return writeFile(path, "mpackage = [[" + package.toUtf8() + "]]\n" + body) && host->getPackageConfig(path) == package;
        };
        QVERIFY(readConfig(qsl("configYes"), "mobile = [[yes]]\n"));
        QVERIFY(readConfig(qsl("configTrue"), "mobile = true\n"));
        QVERIFY(readConfig(qsl("configFalse"), "mobile = false\n"));
        QVERIFY(readConfig(qsl("configOther"), "mobile = [[sometimes]]\n"));
        QVERIFY(readConfig(qsl("configPlain"), "author = [[someone]]\n"));
        QVERIFY(session.packageMobile(qsl("configYes")));
        QVERIFY(session.packageMobile(qsl("configTrue")));
        QVERIFY(!session.packageMobile(qsl("configFalse")));
        QVERIFY(!session.packageMobile(qsl("configOther")));
        QVERIFY(!session.packageMobile(qsl("configPlain")));
        QVERIFY(!session.packageMobile(qsl("noSuchPackage")));
        QCOMPARE(host->mPackageInfo.value(qsl("configTrue")).value(qsl("mobile")), qsl("true"));
        QCOMPARE(host->mPackageInfo.value(qsl("configFalse")).value(qsl("mobile")), qsl("false"));
        QCOMPARE(host->mPackageInfo.value(qsl("configPlain")).keys(), (QStringList{qsl("author"), qsl("mpackage")}));
        QVERIFY(session.shutdown(false));
    }

    void test_aProfileLookIsItsLastSettersAndChangesAreTold()
    {
        const QString name = qsl("Headless Package Ui Looks");
        QVERIFY(writeProfile(name));
        THeadlessSession session;
        QCOMPARE(session.start({.profile = name, .connect = false}), THeadlessSession::StartResult::started);
        using Look = THeadlessSession::ProfileLook;
        QCOMPARE(session.profileLookPackages(Look::borderTop), QStringList());
        QSignalSpy packageUi(&session, &THeadlessSession::packageUiChanged);
        QVERIFY(install(session, qsl("lookPackage"), "lookPackageScript", R"(
setBorderTop(10)
setAppStyleSheet("")
)"));
        QTRY_VERIFY(packageUi.count() >= 1);
        QCOMPARE(session.profileLookPackages(Look::borderTop), QStringList{qsl("lookPackage")});
        QCOMPARE(session.profileLookPackages(Look::appStyleSheet), QStringList{qsl("lookPackage")});
        QCOMPARE(session.profileLookPackages(Look::borderLeft), QStringList());

        packageUi.clear();
        QVERIFY(session.runLua(qsl(R"(setBorderSizes(2))")));
        QTRY_COMPARE(packageUi.count(), 1);
        for (const Look look : {Look::borderTop, Look::borderRight, Look::borderBottom, Look::borderLeft}) {
            QCOMPARE(session.profileLookPackages(look), QStringList());
        }
        QCOMPARE(session.profileLookPackages(Look::appStyleSheet), QStringList{qsl("lookPackage")});

        // Set again by the same code, nothing differs to tell
        packageUi.clear();
        QVERIFY(session.runLua(qsl(R"(setBorderSizes(3))")));
        QTest::qWait(50);
        QCOMPARE(packageUi.count(), 0);
        QVERIFY(session.shutdown(false));
    }
};

#include "HeadlessSessionPackageUiTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessSessionPackageUiTest)
