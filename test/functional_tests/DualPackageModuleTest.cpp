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

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "AliasUnit.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

// One name installed as both a package and a module cannot be made any more -
// installPackage() refuses the second half - but a profile saved before it did
// still holds the combination, and has to go on loading. The item units take
// items away by name alone, so whichever half such a profile removes empties
// both, and uninstallPackage() drops both listings and says so rather than
// leaving one behind with nothing in it.
//
// None of this is reachable from Lua by design, which is what makes it a
// functional test: the only way in is mIsProfileLoadingSequence, the flag the
// profile-loading sequence sets, and no Lua function sets it.
class DualPackageModuleTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-DualPackageModule";
    QString mpPort; // assigned the stub's actual ephemeral port in init()
    const QString mpLocalhost = "localhost";
    const QString mDualName = "dual-spec";

    // Two packages that install under the same name because the name comes from
    // the file, in folders of their own so both can sit in the profile at once.
    // Their aliases are named differently so which half lost its items shows.
    QString writeHalf(const QString& folder, const QString& aliasName)
    {
        const QString home = mudlet::getMudletPath(enums::profileHomePath, mpHostname);
        const QString directory = qsl("%1/%2").arg(home, folder);
        if (!QDir().mkpath(directory)) {
            return QString();
        }
        const QString path = qsl("%1/%2.xml").arg(directory, mDualName);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return QString();
        }
        QTextStream out(&file);
        out << qsl(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
	<TriggerPackage />
	<TimerPackage />
	<AliasPackage>
		<Alias isActive="yes" isFolder="no">
			<name>%1</name>
			<script></script>
			<command></command>
			<packageName></packageName>
			<regex>^%1$</regex>
		</Alias>
	</AliasPackage>
	<ActionPackage />
	<ScriptPackage />
	<KeyPackage />
	<VariablePackage>
		<HiddenVariables />
	</VariablePackage>
</MudletPackage>
)")
                        .arg(aliasName);
        out.flush();
        file.close();
        return path;
    }

    // Every install arms a save, and installPackage() answers a save in progress
    // by postponing itself and returning true - so without waiting the save out
    // the next step is asked for against a profile the previous one never
    // reached, and its true means nothing.
    void settleSaves(Host* host)
    {
        for (int i = 0; i < 200 && (host->hasPendingProfileSave() || host->currentlySavingProfile()); ++i) {
            QTest::qWait(20);
            host->waitForProfileSave();
        }
    }

    QString moduleHalfPath() const { return qsl("%1/as-module/%2.xml").arg(mudlet::getMudletPath(enums::profileHomePath, mpHostname), mDualName); }
    QString packageHalfPath() const { return qsl("%1/as-package/%2.xml").arg(mudlet::getMudletPath(enums::profileHomePath, mpHostname), mDualName); }

    // A profile up and running with both halves written out and neither of them
    // installed, so that a test can ask for either one first.
    Host* startWithBothHalvesWritten()
    {
        startProfile(mpHostname, mpLocalhost, mpPort);
        auto* host = mudlet::self()->getActiveHost();
        if (!host) {
            return nullptr;
        }

        if (writeHalf(qsl("as-package"), qsl("dual-spec package alias")).isEmpty() || writeHalf(qsl("as-module"), qsl("dual-spec module alias")).isEmpty()) {
            return nullptr;
        }

        settleSaves(host);
        return host;
    }

    // Half the legacy state: the package half of the name installed, and the
    // module half written out ready to be asked for.
    Host* startWithThePackageHalf()
    {
        auto* host = startWithBothHalvesWritten();
        if (!host) {
            return nullptr;
        }

        auto [packageInstalled, packageReason] = host->installPackage(packageHalfPath(), enums::PackageModuleType::Package);
        if (!packageInstalled || !host->mInstalledPackages.contains(mDualName)) {
            qWarning() << "DualPackageModuleTest could not install the package half:" << packageReason;
            return nullptr;
        }
        settleSaves(host);
        return host;
    }

    // The other half of it, for the refusal that runs the other way round.
    Host* startWithTheModuleHalf()
    {
        auto* host = startWithBothHalvesWritten();
        if (!host) {
            return nullptr;
        }

        auto [moduleInstalled, moduleReason] = host->installPackage(moduleHalfPath(), enums::PackageModuleType::ModuleFromUI);
        if (!moduleInstalled || !host->mInstalledModules.contains(mDualName)) {
            qWarning() << "DualPackageModuleTest could not install the module half:" << moduleReason;
            return nullptr;
        }
        settleSaves(host);
        return host;
    }

    // The state a profile saved before the refusal existed comes back up in: a
    // package of the name, and a module of the same name that the loading
    // sequence is allowed to install over it.
    Host* buildLegacyDualInstall()
    {
        auto* host = startWithThePackageHalf();
        if (!host) {
            return nullptr;
        }

        host->mIsProfileLoadingSequence = true;
        auto [moduleInstalled, moduleReason] = host->installPackage(moduleHalfPath(), enums::PackageModuleType::ModuleFromUI);
        host->mIsProfileLoadingSequence = false;
        if (!moduleInstalled || !host->mInstalledModules.contains(mDualName)) {
            qWarning() << "DualPackageModuleTest could not install the module half:" << moduleReason;
            return nullptr;
        }

        settleSaves(host);
        return host;
    }

    int aliasCount(Host* host, const QString& name) { return static_cast<int>(host->getAliasUnit()->findItems(name, true, true).size()); }

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void cleanup()
    {
        if (auto* self = mudlet::self()) {
            if (auto* host = self->getActiveHost()) {
                QTest::qWait(50);
                host->waitForProfileSave();
            }
        }
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mpHostname);
        delete mudlet::self();
    }

    // The exemption that lets a legacy profile finish loading is the only way
    // into the combination, and it must not be a way in for anything else: the
    // same install, asked for once the profile is up, has to be refused.
    void test_theCombinationIsOnlyAllowedWhileTheProfileLoads()
    {
        auto* host = startWithThePackageHalf();
        QVERIFY2(host, "Could not install the package half");

        // asked for by a profile that has finished loading, the module half is
        // turned away rather than quietly joined to the package of that name
        auto [installed, reason] = host->installPackage(moduleHalfPath(), enums::PackageModuleType::ModuleFromUI);
        QVERIFY2(!installed, "A module must not be installed over a package of the same name once the profile has loaded");
        QVERIFY2(reason.contains(qsl("A package called")), qPrintable(qsl("The refusal must name the package in the way, but said: \"%1\"").arg(reason)));
        QVERIFY2(!host->mInstalledModules.contains(mDualName), "The refused module was listed anyway");

        // and the very same file, while the profile is loading, goes in: a
        // profile saved before the refusal existed still has to come back up
        settleSaves(host);
        host->mIsProfileLoadingSequence = true;
        auto [loadingInstalled, loadingReason] = host->installPackage(moduleHalfPath(), enums::PackageModuleType::ModuleFromUI);
        host->mIsProfileLoadingSequence = false;
        QVERIFY2(loadingInstalled, qPrintable(qsl("The loading sequence must be allowed to install the module half of a legacy profile, but: \"%1\"").arg(loadingReason)));
        QVERIFY2(host->mInstalledModules.contains(mDualName), "The loading sequence must be allowed to install the module half of a legacy profile");
        QVERIFY2(host->mInstalledPackages.contains(mDualName), "The package half did not survive the module half being installed over it");
    }

    // The same refusal the other way round. Neither half carries a config.lua,
    // so this is the only test that reaches the check running on the archive's
    // own file name - an archive that renames itself is turned away by the
    // second copy of the check further down instead.
    void test_theCombinationIsRefusedWhicheverHalfIsAskedForSecond()
    {
        auto* host = startWithTheModuleHalf();
        QVERIFY2(host, "Could not install the module half");

        auto [installed, reason] = host->installPackage(packageHalfPath(), enums::PackageModuleType::Package);
        QVERIFY2(!installed, "A package must not be installed over a module of the same name once the profile has loaded");
        QVERIFY2(reason.contains(qsl("A module called")), qPrintable(qsl("The refusal must name the module in the way, but said: \"%1\"").arg(reason)));
        QVERIFY2(!host->mInstalledPackages.contains(mDualName), "The refused package was listed anyway");

        settleSaves(host);
        host->mIsProfileLoadingSequence = true;
        auto [loadingInstalled, loadingReason] = host->installPackage(packageHalfPath(), enums::PackageModuleType::Package);
        host->mIsProfileLoadingSequence = false;
        QVERIFY2(loadingInstalled, qPrintable(qsl("The loading sequence must be allowed to install the package half of a legacy profile, but: \"%1\"").arg(loadingReason)));
        QVERIFY2(host->mInstalledPackages.contains(mDualName), "The loading sequence must be allowed to install the package half of a legacy profile");
        QVERIFY2(host->mInstalledModules.contains(mDualName), "The module half did not survive the package half being installed over it");
    }

    // A module can be left listed with nothing behind it - installModulesList()
    // files one even when its install failed - and the refusal above must not
    // hold the name for that: nothing is loaded under it and there are no files,
    // so there is nothing for the package to collide with. Only reachable from
    // here, because Lua has no way to put a listing in without a module.
    void test_aModuleListedWithNothingBehindItDoesNotHoldTheName()
    {
        auto* host = startWithBothHalvesWritten();
        QVERIFY2(host, "Could not start the profile");

        // listed, never loaded, and pointing at a file that is not there
        host->mInstalledModules[mDualName] = QStringList{qsl("%1/gone/%2.mpackage").arg(mudlet::getMudletPath(enums::profileHomePath, mpHostname), mDualName), qsl("0")};
        auto [installed, reason] = host->installPackage(packageHalfPath(), enums::PackageModuleType::Package);
        QVERIFY2(installed, qPrintable(qsl("A listing with no module behind it must not hold the name, but: \"%1\"").arg(reason)));
        QVERIFY2(host->mInstalledPackages.contains(mDualName), "The package was answered yes but not listed");
        QVERIFY2(!host->mInstalledModules.contains(mDualName), "The leftover listing was left in place for the next install to trip over");
    }

    // Removing either half takes both halves' items with it whatever is asked
    // for, so the half that was not named is dropped as well and the user is
    // told - rather than being left a listing with nothing behind it.
    void test_uninstallingOneHalfRemovesBothAndSaysSo()
    {
        auto* host = buildLegacyDualInstall();
        QVERIFY2(host, "Could not build the legacy dual install");
        QCOMPARE(aliasCount(host, qsl("dual-spec package alias")), 1);
        QCOMPARE(aliasCount(host, qsl("dual-spec module alias")), 1);

        QVERIFY2(host->uninstallPackage(mDualName, enums::PackageModuleType::Package), "The uninstall was refused");
        QApplication::processEvents();

        QVERIFY2(!host->mInstalledPackages.contains(mDualName), "The package half stayed listed");
        QVERIFY2(!host->mInstalledModules.contains(mDualName), "The module half stayed listed after its items were destroyed with the package's");
        QCOMPARE(aliasCount(host, qsl("dual-spec package alias")), 0);
        QCOMPARE(aliasCount(host, qsl("dual-spec module alias")), 0);
        QVERIFY2(bufferContains(qsl("was installed as both a package and a module")), qPrintable(qsl("Removing both halves has to be announced, but the buffer held: \"%1\"").arg(joinedBuffer())));
    }

    // A module sync is a temporary uninstall, but the items it takes away are
    // still taken away by name, so it empties the package half too. Returning
    // early from it used to leave that half listed with nothing in it and
    // nothing said - the very state the uninstall above exists to prevent.
    void test_aModuleSyncDoesNotLeaveThePackageHalfListedAndEmpty()
    {
        auto* host = buildLegacyDualInstall();
        QVERIFY2(host, "Could not build the legacy dual install");
        QCOMPARE(aliasCount(host, qsl("dual-spec package alias")), 1);

        QVERIFY2(host->uninstallPackage(mDualName, enums::PackageModuleType::ModuleSync), "The sync's uninstall was refused");
        QApplication::processEvents();

        QCOMPARE(aliasCount(host, qsl("dual-spec package alias")), 0);
        QVERIFY2(!host->mInstalledPackages.contains(mDualName), "A module sync emptied the package half and left it listed");
        QVERIFY2(bufferContains(qsl("was installed as both a package and a module")),
                 qPrintable(qsl("A sync that removes the package half has to say so, but the buffer held: \"%1\"").arg(joinedBuffer())));
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    QString joinedBuffer()
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        QString allText;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            allText.append(console->buffer.line(i)).append(QChar::Space);
        }
        return allText.simplified();
    }

    bool bufferContains(const QString& needle) { return joinedBuffer().contains(needle); }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

#include "DualPackageModuleTest.moc"
MUDLET_GROUPED_TEST_MAIN(DualPackageModuleTest)
