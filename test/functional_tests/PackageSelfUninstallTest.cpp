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
 * Regression test for use-after-free when a package uninstalls itself from its
 * own timer script or event-handler script (#9337 class, the remaining timer
 * and script cases).
 *
 * Package auto-updaters commonly call uninstallPackage()+installPackage() on
 * their own package from one of the package's own items. Since the
 * *Unit::uninstall() methods started deleting items immediately (instead of
 * just unregistering them), doing that from a timer deleted the very TTimer
 * whose execute() was still on the call stack, and doing it from an event
 * handler deleted TScript objects that Host::raiseEvent() was still iterating
 * over - heap corruption either way. TriggerUnit/AliasUnit/KeyUnit gained a
 * processing-depth deferral in #9383; this covers TimerUnit and ScriptUnit.
 *
 * Under AddressSanitizer the pre-fix code aborts with heap-use-after-free
 * inside TTimer::execute() / Host::raiseEvent(); with the deferral in place
 * both scenarios complete cleanly.
 *
 * Run with: ctest -R PackageSelfUninstallTest -V
 */

#include <QtTest/QtTest>

#include <QTemporaryDir>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "ScriptUnit.h"
#include "TEvent.h"
#include "TScript.h"
#include "TTimer.h"
#include "TimerUnit.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForPackageSelfUninstallTest();

class PackageSelfUninstallTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("PackageSelfUninstall-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForPackageSelfUninstallTest();

        // Keep the test hermetic: point the config dir resolution at a
        // temporary directory instead of the user's real profiles.
        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QVERIFY2(mudlet::self()->getHostManager().addHost(mProfileName, QString(), QString(), QString()), "failed to create the Host");
        mpHost = mudlet::self()->getHostManager().getHost(mProfileName);
        QVERIFY(mpHost);
        // A bare Host blocks script compilation until the full profile boot
        // would normally clear this; the test's scripts need to compile:
        mpHost->mBlockScriptCompile = false;
        // NB: mLoadedOk is left false on purpose - the deferred saveProfile()
        // that uninstallPackage() schedules then declines to run, which this
        // console-less test Host could not service anyway.
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A package's event-handler script uninstalls its own package while
    // Host::raiseEvent() is mid-dispatch. The second handler in the same
    // package is what the pre-fix code would have called through a freed
    // TScript pointer.
    void test_scriptEventHandlerSelfUninstall()
    {
        const QString packageName = qsl("selfuninstall-script");
        mpHost->mInstalledPackages << packageName;

        auto pUninstaller = new TScript(nullptr, mpHost);
        mpHost->getScriptUnit()->registerScript(pUninstaller);
        pUninstaller->mPackageName = packageName;
        pUninstaller->setName(qsl("selfUninstallHandler"));
        QVERIFY2(pUninstaller->setScript(qsl("function selfUninstallHandler(event)\n    uninstallPackage(\"%1\")\nend\n").arg(packageName)), "uninstaller handler script failed to compile");
        pUninstaller->setEventHandlerList(QStringList{qsl("testSelfUninstallEvent")});
        pUninstaller->setIsActive(true);

        auto pBystander = new TScript(nullptr, mpHost);
        mpHost->getScriptUnit()->registerScript(pBystander);
        pBystander->mPackageName = packageName;
        pBystander->setName(qsl("selfUninstallBystander"));
        QVERIFY2(pBystander->setScript(qsl("function selfUninstallBystander(event)\nend\n")), "bystander handler script failed to compile");
        pBystander->setEventHandlerList(QStringList{qsl("testSelfUninstallEvent")});
        pBystander->setIsActive(true);

        TEvent event{};
        event.mArgumentList.append(qsl("testSelfUninstallEvent"));
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        // Pre-fix this dispatch freed both TScripts under raiseEvent()'s feet:
        mpHost->raiseEvent(event);

        QVERIFY2(!mpHost->mInstalledPackages.contains(packageName), "package was not uninstalled");
        // The deferred deletes must have been flushed once the dispatch ended:
        QVERIFY2(mpHost->getScriptUnit()->findItems(qsl("selfUninstallHandler")).empty(), "uninstalled script is still registered");
        QVERIFY2(mpHost->getScriptUnit()->findItems(qsl("selfUninstallBystander")).empty(), "uninstalled script is still registered");
    }

    // A package's timer script uninstalls its own package while
    // TTimer::execute() is still on the call stack for that timer. Pre-fix,
    // execute() would resume on a freed `this`.
    void test_timerScriptSelfUninstall()
    {
        const QString packageName = qsl("selfuninstall-timer");
        mpHost->mInstalledPackages << packageName;

        auto pTimer = new TTimer(qsl("selfUninstallTimer"), QTime(0, 0, 0, 250), mpHost);
        mpHost->getTimerUnit()->registerTimer(pTimer);
        pTimer->mPackageName = packageName;
        QVERIFY2(pTimer->setScript(qsl("uninstallPackage(\"%1\")").arg(packageName)), "timer script failed to compile");
        pTimer->setIsActive(true);
        pTimer->enableTimer();

        // Let the timer fire and take its package (and itself) down:
        QTRY_VERIFY_WITH_TIMEOUT(!mpHost->mInstalledPackages.contains(packageName), 5000);

        // Allow any queued activity (the declined deferred save, further timer
        // ticks) to surface problems:
        QTest::qWait(500);
        // mudlet::slot_timerFires() flushes the deferred delete as soon as the
        // uninstalling timer's execute() has finished, so by now the timer must
        // be properly gone - not lingering deactivated where the next profile
        // save would serialize it back in:
        QVERIFY2(!mpHost->getTimerUnit()->findFirstTimer(qsl("selfUninstallTimer")), "uninstalled package timer is still registered");
    }
};

void initializeQRCResourcesForPackageSelfUninstallTest()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "PackageSelfUninstallTest.moc"
QTEST_MAIN(PackageSelfUninstallTest)
