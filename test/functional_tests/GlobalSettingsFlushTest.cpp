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

#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostDialogs.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgProfilePreferences.h"
#include "dlgTriggerEditor.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Most application-wide settings are kept in memory until Mudlet quits, so
// every way of saving a profile has to write Mudlet.ini as well, or a crash
// after it loses them while the profile's own half survives.
class GlobalSettingsFlushTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QStringList mHostnames{qsl("GlobalSettingsFlush-First"), qsl("GlobalSettingsFlush-Second"), qsl("GlobalSettingsFlush-Third")};
    const QString mKey = qsl("showTabConnectionIndicators");

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    // Read from the file itself: another QSettings on the same file would share
    // the application's unsynced cache and see values that never reached disk
    static QString onDisk(const QString& key)
    {
        QFile file(MudletApp::getQSettings()->fileName());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        const QString prefix = key + qsl("=");
        while (!file.atEnd()) {
            const QString line = QString::fromUtf8(file.readLine()).trimmed();
            if (line.startsWith(prefix)) {
                return line.mid(prefix.size());
            }
        }
        return QString();
    }

    // The opposite of what Mudlet.ini holds, so only the save under test can put it there
    QString flipAwayFromDisk() const
    {
        const bool wanted = onDisk(mKey) != qsl("true");
        mudlet::self()->setShowTabConnectionIndicators(wanted);
        return wanted ? qsl("true") : qsl("false");
    }

    Host* host(const int index) const { return HostManager::self()->getHost(mHostnames.at(index)); }

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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->show();
        for (const auto& name : mHostnames) {
            startProfile(name);
            if (QTest::currentTestFailed()) {
                return;
            }
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            for (const auto& name : mHostnames) {
                QDir(MudletApp::getMudletPath(enums::profileHomePath, name)).removeRecursively();
            }
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_editorSaveProfile()
    {
        Host* pHost = host(0);
        QVERIFY(pHost);
        dlgTriggerEditor* pEditor = editorFor(pHost);
        QVERIFY(pEditor);
        QTRY_VERIFY_WITH_TIMEOUT(!pHost->currentlySavingProfile(), 10s);
        const QString wanted = flipAwayFromDisk();

        pEditor->slot_profileSaveAction();

        QTRY_VERIFY_WITH_TIMEOUT(!pHost->currentlySavingProfile(), 10s);
        QCOMPARE(onDisk(mKey), wanted);
    }

    void test_editorSaveProfileAs()
    {
        Host* pHost = host(0);
        QVERIFY(pHost);
        dlgTriggerEditor* pEditor = editorFor(pHost);
        QVERIFY(pEditor);
        QTRY_VERIFY_WITH_TIMEOUT(!pHost->currentlySavingProfile(), 10s);
        const QString wanted = flipAwayFromDisk();
        const QString target = mConfigDir.filePath(qsl("backup.xml"));
        bool dialogAnswered = false;
        int attempts = 0;
        std::function<void()> answer = [&]() {
            for (auto* pWidget : QApplication::topLevelWidgets()) {
                if (auto* pDialog = qobject_cast<QFileDialog*>(pWidget); pDialog && pDialog->isVisible()) {
                    if (auto* pFileName = pDialog->findChild<QLineEdit*>(qsl("fileNameEdit"))) {
                        pFileName->setText(target);
                    }
                    QMetaObject::invokeMethod(pDialog, "accept", Qt::QueuedConnection);
                    dialogAnswered = true;
                    return;
                }
            }
            if (++attempts < 50) {
                QTimer::singleShot(100ms, answer);
            }
        };
        QTimer::singleShot(100ms, answer);
        // A native dialog would block without running the timer that answers it
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);

        pEditor->slot_profileSaveAsAction();

        QVERIFY2(dialogAnswered, "the Save Profile As file dialog never showed up");
        QTRY_VERIFY_WITH_TIMEOUT(!pHost->currentlySavingProfile(), 10s);
        QVERIFY2(QFileInfo::exists(target), "Save Profile As did not write the backup");
        QCOMPARE(onDisk(mKey), wanted);
    }

    void test_closingPreferences()
    {
        Host* pHost = host(0);
        QVERIFY(pHost);
        QTRY_VERIFY_WITH_TIMEOUT(!pHost->currentlySavingProfile(), 10s);
        auto* pPreferences = new dlgProfilePreferences(mudlet::self(), pHost);
        pPreferences->setAttribute(Qt::WA_DeleteOnClose);
        pPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(pPreferences));
        const QString wanted = flipAwayFromDisk();

        pPreferences->close();

        QTRY_VERIFY_WITH_TIMEOUT(!pHost->currentlySavingProfile(), 10s);
        QCOMPARE(onDisk(mKey), wanted);
    }

    // Closing one profile leaves Mudlet running for the others, so the quit
    // that would otherwise write Mudlet.ini may be a long way off
    void test_closingAProfileAndSavingIt()
    {
        Host* pHost = host(2);
        QVERIFY(pHost);
        pHost->mFORCE_SAVE_ON_EXIT = false;
        const QString wanted = flipAwayFromDisk();
        bool answered = false;
        int attempts = 0;
        std::function<void()> answer = [&]() {
            if (auto* pBox = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
                if (auto* pYes = pBox->button(QMessageBox::Yes)) {
                    answered = true;
                    pYes->click();
                    return;
                }
            }
            if (++attempts < 50) {
                QTimer::singleShot(100ms, answer);
            }
        };
        QTimer::singleShot(100ms, answer);

        mudlet::self()->slot_closeProfileRequested(mudlet::self()->mpTabBar->tabIndex(mHostnames.at(2)));

        QVERIFY2(answered, "the \"Save profile?\" question never showed up");
        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self()->getHost(mHostnames.at(2)), 10s);
        QCOMPARE(onDisk(mKey), wanted);
    }

    void test_closingAProfileThatSavesOnExit()
    {
        Host* pHost = host(1);
        QVERIFY(pHost);
        pHost->mFORCE_SAVE_ON_EXIT = true;
        const QString wanted = flipAwayFromDisk();

        mudlet::self()->slot_closeProfileRequested(mudlet::self()->mpTabBar->tabIndex(mHostnames.at(1)));

        QTRY_VERIFY_WITH_TIMEOUT(!HostManager::self()->getHost(mHostnames.at(1)), 10s);
        QCOMPARE(onDisk(mKey), wanted);
    }

private:
    // slot_showScriptDialog() opens the editor of the profile in front
    dlgTriggerEditor* editorFor(Host* pHost) const
    {
        if (!HostDialogs::of(pHost).mpEditorDialog) {
            mudlet::self()->activateProfile(pHost);
            QTest::qWait(100ms);
            mudlet::self()->slot_showScriptDialog();
            QTest::qWait(100ms);
        }
        return HostDialogs::of(pHost).mpEditorDialog.data();
    }

    void startProfile(const QString& hostname)
    {
        auto pHost = TestProfile::create(hostname, mLocalhost, mPort);
        if (!pHost) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connectionSpy(&(pHost->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
    }
};

#include "GlobalSettingsFlushTest.moc"
MUDLET_GROUPED_TEST_MAIN(GlobalSettingsFlushTest)
