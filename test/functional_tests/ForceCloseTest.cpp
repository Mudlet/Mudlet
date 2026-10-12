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
 * closeMudlet() walks every open profile, and each one pumps the event loop to
 * show its closing message; whatever that pump runs may close a profile, and
 * the walk must survive the pool changing underneath it.
 */

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "mudlet.h"

#include "GroupedTest.h"

#include <QApplication>
#include <QDialog>
#include <QPointer>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace std::chrono_literals;

class ForceCloseTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    const QStringList mProfileNames{qsl("ForceClose-Test-1"), qsl("ForceClose-Test-2"), qsl("ForceClose-Test-3")};
    QPointer<mudlet> mWindow;

    static bool provisionProfileOnDisk(const QString& name)
    {
        return QDir().mkpath(MudletApp::getMudletPath(enums::profileHomePath, name)) && MudletApp::writeProfileData(name, qsl("url"), qsl("localhost")).first
               && MudletApp::writeProfileData(name, qsl("port"), qsl("23")).first;
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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        // Keeps the first-run UI tour and the starter UI package out of this test
        MudletApp::getQSettings()->setValue(qsl("uiTourShown"), true);
        MudletApp::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QVERIFY2(mudlet::self()->experiencedMudletPlayer(), "the first-run UI would open over this test");
        mWindow = mudlet::self();
    }

    // Already null when the case let the window close; a failed case leaves it
    void cleanup() { delete mudlet::self(); }

    // The tab close button, or a detached window's, closes the profile on a
    // posted event, which forceClose()'s pump for the closing message delivers
    void test_aProfileCloseStillPendingDoesNotBreakTheWalk()
    {
        for (const auto& name : mProfileNames) {
            QVERIFY(provisionProfileOnDisk(name));
        }
        mWindow->startAutoLogin(mProfileNames, true);
        QList<QPointer<Host>> hosts;
        for (const auto& name : mProfileNames) {
            hosts.append(HostManager::self()->getHost(name));
            QVERIFY2(hosts.last(), qPrintable(qsl("%1 did not load").arg(name)));
        }

        // The first profile the walk visits, so its own pump removes the entry it stands on
        mWindow->slot_closeProfileByName(mProfileNames.first());
        QVERIFY2(HostManager::self()->getHost(mProfileNames.first()), "the profile closed at once, so nothing was left for the walk to meet");
        mWindow->forceClose();

        QTRY_VERIFY2_WITH_TIMEOUT(mWindow.isNull(), "forceClose() never closed the window", 30s);
        for (const auto& host : std::as_const(hosts)) {
            QVERIFY2(host.isNull(), "the window closed but left a profile loaded");
        }
    }

    // A telnet URI handed over by another instance loads its profile on an event
    // that forceClose()'s pump may deliver, after the walk took its snapshot
    void test_aProfileLoadedDuringTheWalkIsForcedClosedToo()
    {
        const QString early = mProfileNames.first();
        const QString late = mProfileNames.last();
        QVERIFY(provisionProfileOnDisk(early));
        QVERIFY(provisionProfileOnDisk(late));
        mWindow->startAutoLogin({early}, true);
        QVERIFY2(HostManager::self()->getHost(early), "the first profile did not load");

        QPointer<Host> lateHost;
        bool lateLoaded = false;
        QTimer::singleShot(0, mWindow, [this, &late, &lateHost, &lateLoaded]() {
            mWindow->startAutoLogin({late}, true);
            lateHost = HostManager::self()->getHost(late);
            if (lateHost) {
                lateLoaded = true;
                lateHost->mFORCE_SAVE_ON_EXIT = false;
            }
        });

        // Without save on exit an unforced profile asks whether to save it, which would hold the close open
        QString askedTitle;
        QTimer modalWatch;
        connect(&modalWatch, &QTimer::timeout, this, [&askedTitle]() {
            if (auto dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
                if (askedTitle.isEmpty()) {
                    askedTitle = dialog->windowTitle();
                }
                dialog->reject();
            }
        });
        modalWatch.start(50);

        mWindow->forceClose();

        QTRY_VERIFY2_WITH_TIMEOUT(mWindow.isNull() || !askedTitle.isEmpty(), "forceClose() never closed the window", 30s);
        QVERIFY2(lateLoaded, "the late profile did not load during the walk, so nothing was tested");
        QVERIFY2(askedTitle.isEmpty(), qPrintable(qsl("forceClose() asked \"%1\" about a profile it should have forced closed").arg(askedTitle)));
        QVERIFY2(mWindow.isNull(), "forceClose() did not close the window");
        QVERIFY2(lateHost.isNull(), "the window closed but left the late profile loaded");
    }
};

#include "ForceCloseTest.moc"
MUDLET_GROUPED_TEST_MAIN(ForceCloseTest)
